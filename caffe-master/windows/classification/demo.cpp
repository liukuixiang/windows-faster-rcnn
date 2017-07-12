#include"fasterDemo.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "yuStdDirFiles.h"

int main()
{
	string strNetStructPath = "F:\\faster_rcnn\\model\\faster_rcnn_test_zf.pt";
	//string strNetStructPath = "/home/dl/py-faster-rcnn-master/faster_rcnn_C++_demo/model/faster_rcnn_test.pt";
	string strNetWeightPath = "F:\\faster_rcnn\\model\\ZF_faster_rcnn_final.caffemodel";
	int GPU_ID = 0;    //GPU ID
	float CONF_THRESH = 0.5;    //框的置信度
	float NMS_THRESH = 0.3;    //NMS置信度

	ObjectDetector fasterDetect;

	fasterDetect.Detector(strNetStructPath,strNetWeightPath,GPU_ID);
	
	string strImgPath = "F:\\faster_rcnn\\model\\testimage\\";
	std::cout << "strImgPath= " << strImgPath << endl;
	string extensions[] = { ".jpg", ".png" };
	vector<string>	img_extensions(extensions, extensions + 2);
	vector<string>	imgNames = yuStdDirFiles(strImgPath, img_extensions);
	if (imgNames.size() == 0)
	{
		printf("No images found.\n");
		return -1;
	}
	for (unsigned int i = 0; i < imgNames.size(); i++)
	{
		printf("[%d]:%s\n", i, imgNames[i].c_str());
		cv::Mat frame = cv::imread(imgNames[i]);
		if (frame.empty())
			continue;
		double ratio = 1;
		if (frame.rows > 720)
		{
			ratio = 720.0 / frame.rows;
			resize(frame, frame, cv::Size(), ratio, ratio);
		}

		double t_total = (double)cvGetTickCount();
		vector< vector<float> > vec2d_bbs;
		if (fasterDetect.detect(frame, vec2d_bbs, CONF_THRESH, NMS_THRESH))
		{
			for (unsigned int i = 0; i < vec2d_bbs.size(); i++)
			{
				cv::rectangle(frame,
					cv::Point((int)vec2d_bbs[i][1], (int)vec2d_bbs[i][2]),
					cv::Point((int)vec2d_bbs[i][3], (int)vec2d_bbs[i][4]),
					cv::Scalar(0, 0, 255), 2);

				char buffer[20];
				sprintf(buffer, "%.1f%%", vec2d_bbs[i][5]*100);
				string str = buffer;
				putText(frame, str, cv::Point((int)vec2d_bbs[i][1], (int)vec2d_bbs[i][2] - 1),
					cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0),2);
			}
		}
		t_total = (double)cvGetTickCount() - t_total;
		printf("   >>total time consume %.4f s\n", t_total / ((double)cvGetTickFrequency()*1e+6));
		cv::imshow("output", frame);
		if (cv::waitKey(0) == 27)
			break;
	}

	return 0;
}

