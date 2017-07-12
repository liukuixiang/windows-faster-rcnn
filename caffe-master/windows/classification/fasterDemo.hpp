#ifndef FASTER_DEMO_HPP_
#define FASTER_DEMO_HPP_

#define INPUT_SIZE_NARROW  600
#define INPUT_SIZE_LONG  1000

#include <string>
#include <caffe/net.hpp>
#include <caffe/common.hpp>
#include <opencv2/core/core.hpp>
#include <iostream>
#include <memory>
#include <map>

using namespace std;

class ObjectDetector
{
public:

    bool Detector(const string &model_file, const string &weights_file,int GPUID);  //���ع��캯��
	bool detect(cv::Mat& image, vector< vector<float> >& vec2d_bbs,float CONF_THRESH,float NMS_THRESH);

private:
	boost::shared_ptr< caffe::Net<float> > net_;
	int class_num_;     //�����+1 
};

#endif