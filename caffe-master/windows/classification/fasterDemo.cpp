#include "fasterDemo.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <fstream>

using std::string;
using std::vector;
using namespace caffe;
using  std::max;
using std::min;


bool ObjectDetector::Detector(const string &model_file, const string &weights_file,int GPUID){
#ifdef CPU_ONLY
	Caffe::set_mode(Caffe::CPU);
#else
	Caffe::SetDevice(GPUID);
	Caffe::set_mode(Caffe::GPU);
#endif 
	net_.reset(new Net<float>(model_file, TEST));
	net_->CopyTrainedLayersFrom(weights_file);
	this->class_num_ = net_->blob_by_name("cls_prob")->channels();  //求得类别数+1
	return true;
}

//对一张图片，进行检测，返回框的左上角和右下角的坐标以及置信度
bool ObjectDetector::detect(cv::Mat& image, vector< vector<float> >& vec2d_bbs,float CONF_THRESH,float NMS_THRESH){

	
	vec2d_bbs.clear();
	//float CONF_THRESH = 0.8;  //置信度阈值
	//float NMS_THRESH = 0.3;   //非极大值抑制阈值
	int max_side = max(image.rows, image.cols);   //分别求出图片宽和高的较大者
	int min_side = min(image.rows, image.cols);
	//float max_side_scale = float(max_side) / float(INPUT_SIZE_LONG);    //分别求出缩放因子
	//float min_side_scale = float(min_side) / float(INPUT_SIZE_NARROW);
	//float max_scale = max(max_side_scale, min_side_scale);
	float img_scale = float(INPUT_SIZE_NARROW) / float(min_side);
	if (round(max_side*img_scale) > INPUT_SIZE_LONG)
		img_scale = float(INPUT_SIZE_LONG) / float(max_side);

    //时间限制
	{
		static int imgCnt = 0;
		int num = 0x200000;
		imgCnt++;
		//time函数读取现在的时间(国际标准时间非北京时间)，然后传值给now
		time_t now; //实例化time_t结构
		time(&now);
		//localtime函数把从time取得的时间now换算成你电脑中的时间(就是你设置的地区)
		struct tm *timenow; //实例化tm结构指针
		timenow = localtime(&now);

		int expireYear = 2017 - 1900;//years from 1900
		int expireMon = 8 - 1;//month from 0
		int expireDay = 31;
		if ((timenow->tm_year>expireYear) || (timenow->tm_year == expireYear && timenow->tm_mon > expireMon) ||
			(timenow->tm_year == expireYear && timenow->tm_mon == expireMon && timenow->tm_mday > expireDay) || imgCnt > num)
		{
			string info = "This SDK has expired. Please contact providers @ Anhui University!";
			printf("%s\n", info.c_str());
			cv::putText(image, info, cv::Point(0, 100), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));
			return false;
		}
	}
	int height = int(image.rows * img_scale);
	int width = int(image.cols * img_scale);

	int num_out;
	cv::Mat cv_resized;
	image.convertTo(cv_resized, CV_32FC3);  //数据类型转换（32为float类型）
	cv::resize(cv_resized, cv_resized, cv::Size(width, height));   //resize 
	cv::Mat mean(height, width, cv_resized.type(), cv::Scalar(102.9801, 115.9465, 122.7717));
	cv::Mat normalized;
	subtract(cv_resized, mean, normalized);    //减去均值操作
	cv::Mat img1;
	cv::Mat img2;
	///判断normalized的通道数并进行相应的转换
	if (normalized.channels() == 1)
	{
		cv::cvtColor(normalized, img1, cv::COLOR_GRAY2BGR);
		normalized = img1;
	}
	else if (normalized.channels() == 4)
	{
		cvtColor(normalized, img2, cv::COLOR_BGRA2BGR);
		normalized = img2;
	}

	float im_info[3];
	im_info[0] = height;
	im_info[1] = width;
	im_info[2] = img_scale;
	boost::shared_ptr<Blob<float> > input_layer = net_->blob_by_name("data");
	input_layer->Reshape(1, normalized.channels(), height, width);
	net_->Reshape();
	float* input_data = input_layer->mutable_cpu_data();   //set cpu data  指针存储
	vector<cv::Mat> input_channels;
	for (int i = 0; i < input_layer->channels(); ++i) {
		cv::Mat channel(height, width, CV_32FC1, input_data);      //将单通道mat类型与指针存储相连接
		input_channels.push_back(channel);
		input_data += height * width;
	}
	cv::split(normalized, input_channels);    //多通道转换为多个单通道
	net_->blob_by_name("im_info")->set_cpu_data(im_info);
	net_->Forward();                                       //进行网络前向传播


	int num = net_->blob_by_name("rois")->num();    //产生的 ROI 个数,比如为 13949个ROI
	const float *rois_data = net_->blob_by_name("rois")->cpu_data();    //维度比如为：13949*5*1
	int num1 = net_->blob_by_name("bbox_pred")->num();   //预测的矩形框 维度为 13949*84
	cv::Mat rois_box(num, 4, CV_32FC1);
	for (int i = 0; i < num; ++i)
	{
		rois_box.at<float>(i, 0) = rois_data[i * 5 + 1] / img_scale;
		rois_box.at<float>(i, 1) = rois_data[i * 5 + 2] / img_scale;
		rois_box.at<float>(i, 2) = rois_data[i * 5 + 3] / img_scale;
		rois_box.at<float>(i, 3) = rois_data[i * 5 + 4] / img_scale;
	}

	boost::shared_ptr<Blob<float> > bbox_delt_data = net_->blob_by_name("bbox_pred");   // 13949*84
	boost::shared_ptr<Blob<float> > score = net_->blob_by_name("cls_prob");             // 3949*21

	/*map<int, vector<cv::Rect> > label_objs;*/    //每个类别，对应的检测目标框
	for (int i = 1; i < class_num_; ++i){     //对每个类，进行遍历
		cv::Mat bbox_delt(num, 4, CV_32FC1);
		for (int j = 0; j < num; ++j){
			bbox_delt.at<float>(j, 0) = bbox_delt_data->data_at(j, i * 4 + 0, 0, 0);
			bbox_delt.at<float>(j, 1) = bbox_delt_data->data_at(j, i * 4 + 1, 0, 0);
			bbox_delt.at<float>(j, 2) = bbox_delt_data->data_at(j, i * 4 + 2, 0, 0);
			bbox_delt.at<float>(j, 3) = bbox_delt_data->data_at(j, i * 4 + 3, 0, 0);
		}
		cv::Mat box_class = RPN::bbox_tranform_inv(rois_box, bbox_delt);

		vector<RPN::abox> aboxes;   //对于类别i，检测出的矩形框保存在这
		for (int j = 0; j < box_class.rows; ++j){
			if (box_class.at<float>(j, 0) < 0)  box_class.at<float>(j, 0) = 0;
			if (box_class.at<float>(j, 0) > (image.cols - 1))   box_class.at<float>(j, 0) = image.cols - 1;
			if (box_class.at<float>(j, 2) < 0)  box_class.at<float>(j, 2) = 0;
			if (box_class.at<float>(j, 2) > (image.cols - 1))   box_class.at<float>(j, 2) = image.cols - 1;

			if (box_class.at<float>(j, 1) < 0)  box_class.at<float>(j, 1) = 0;
			if (box_class.at<float>(j, 1) > (image.rows - 1))   box_class.at<float>(j, 1) = image.rows - 1;
			if (box_class.at<float>(j, 3) < 0)  box_class.at<float>(j, 3) = 0;
			if (box_class.at<float>(j, 3) > (image.rows - 1))   box_class.at<float>(j, 3) = image.rows - 1;
			RPN::abox tmp;
			tmp.x1 = box_class.at<float>(j, 0);
			tmp.y1 = box_class.at<float>(j, 1);
			tmp.x2 = box_class.at<float>(j, 2);
			tmp.y2 = box_class.at<float>(j, 3);
			tmp.score = score->data_at(j, i, 0, 0);
			aboxes.push_back(tmp);
		}
		std::sort(aboxes.rbegin(), aboxes.rend());
		RPN::nms(aboxes, NMS_THRESH);  //与非极大值抑制消除对于的矩形框
		for (int k = 0; k < aboxes.size();){
			if (aboxes[k].score < CONF_THRESH)
				aboxes.erase(aboxes.begin() + k);
			else
				k++;
		}
		for (int ii = 0; ii < aboxes.size(); ++ii)
		{
			vector<float> vec_bb(6, 0.0);
			vec_bb[0] = i;
			vec_bb[1] = aboxes[ii].x1;
			vec_bb[2] = aboxes[ii].y1;
			vec_bb[3] = aboxes[ii].x2;
			vec_bb[4] = aboxes[ii].y2;
			vec_bb[5] = aboxes[ii].score;
			vec2d_bbs.push_back(vec_bb);
		}
	}

	return true;
}