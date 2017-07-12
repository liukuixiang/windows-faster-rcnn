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
	this->class_num_ = net_->blob_by_name("cls_prob")->channels();  //��������+1
	return true;
}

//��һ��ͼƬ�����м�⣬���ؿ�����ϽǺ����½ǵ������Լ����Ŷ�
bool ObjectDetector::detect(cv::Mat& image, vector< vector<float> >& vec2d_bbs,float CONF_THRESH,float NMS_THRESH){

	
	vec2d_bbs.clear();
	//float CONF_THRESH = 0.8;  //���Ŷ���ֵ
	//float NMS_THRESH = 0.3;   //�Ǽ���ֵ������ֵ
	int max_side = max(image.rows, image.cols);   //�ֱ����ͼƬ��͸ߵĽϴ���
	int min_side = min(image.rows, image.cols);
	//float max_side_scale = float(max_side) / float(INPUT_SIZE_LONG);    //�ֱ������������
	//float min_side_scale = float(min_side) / float(INPUT_SIZE_NARROW);
	//float max_scale = max(max_side_scale, min_side_scale);
	float img_scale = float(INPUT_SIZE_NARROW) / float(min_side);
	if (round(max_side*img_scale) > INPUT_SIZE_LONG)
		img_scale = float(INPUT_SIZE_LONG) / float(max_side);

    //ʱ������
	{
		static int imgCnt = 0;
		int num = 0x200000;
		imgCnt++;
		//time������ȡ���ڵ�ʱ��(���ʱ�׼ʱ��Ǳ���ʱ��)��Ȼ��ֵ��now
		time_t now; //ʵ����time_t�ṹ
		time(&now);
		//localtime�����Ѵ�timeȡ�õ�ʱ��now�����������е�ʱ��(���������õĵ���)
		struct tm *timenow; //ʵ����tm�ṹָ��
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
	image.convertTo(cv_resized, CV_32FC3);  //��������ת����32Ϊfloat���ͣ�
	cv::resize(cv_resized, cv_resized, cv::Size(width, height));   //resize 
	cv::Mat mean(height, width, cv_resized.type(), cv::Scalar(102.9801, 115.9465, 122.7717));
	cv::Mat normalized;
	subtract(cv_resized, mean, normalized);    //��ȥ��ֵ����
	cv::Mat img1;
	cv::Mat img2;
	///�ж�normalized��ͨ������������Ӧ��ת��
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
	float* input_data = input_layer->mutable_cpu_data();   //set cpu data  ָ��洢
	vector<cv::Mat> input_channels;
	for (int i = 0; i < input_layer->channels(); ++i) {
		cv::Mat channel(height, width, CV_32FC1, input_data);      //����ͨ��mat������ָ��洢������
		input_channels.push_back(channel);
		input_data += height * width;
	}
	cv::split(normalized, input_channels);    //��ͨ��ת��Ϊ�����ͨ��
	net_->blob_by_name("im_info")->set_cpu_data(im_info);
	net_->Forward();                                       //��������ǰ�򴫲�


	int num = net_->blob_by_name("rois")->num();    //������ ROI ����,����Ϊ 13949��ROI
	const float *rois_data = net_->blob_by_name("rois")->cpu_data();    //ά�ȱ���Ϊ��13949*5*1
	int num1 = net_->blob_by_name("bbox_pred")->num();   //Ԥ��ľ��ο� ά��Ϊ 13949*84
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

	/*map<int, vector<cv::Rect> > label_objs;*/    //ÿ����𣬶�Ӧ�ļ��Ŀ���
	for (int i = 1; i < class_num_; ++i){     //��ÿ���࣬���б���
		cv::Mat bbox_delt(num, 4, CV_32FC1);
		for (int j = 0; j < num; ++j){
			bbox_delt.at<float>(j, 0) = bbox_delt_data->data_at(j, i * 4 + 0, 0, 0);
			bbox_delt.at<float>(j, 1) = bbox_delt_data->data_at(j, i * 4 + 1, 0, 0);
			bbox_delt.at<float>(j, 2) = bbox_delt_data->data_at(j, i * 4 + 2, 0, 0);
			bbox_delt.at<float>(j, 3) = bbox_delt_data->data_at(j, i * 4 + 3, 0, 0);
		}
		cv::Mat box_class = RPN::bbox_tranform_inv(rois_box, bbox_delt);

		vector<RPN::abox> aboxes;   //�������i�������ľ��ο򱣴�����
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
		RPN::nms(aboxes, NMS_THRESH);  //��Ǽ���ֵ�����������ڵľ��ο�
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