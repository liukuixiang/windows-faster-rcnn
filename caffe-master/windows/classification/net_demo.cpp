#include <vector>
#include <iostream>
#include <caffe/net.hpp>
using namespace std;
using namespace caffe;

int main(void)
{
	std::string proto("F:\\faster_rcnn\\model\\faster_rcnn_test_zf.pt");
	Net<float>nn(proto, caffe::TEST);
	vector<string>bn = nn.blob_names();
	for (int i = 0; i < bn.size(); i++)
	{
		cout << "Blob #" << i << ":" << bn[i] << endl;
	}
	return 0;

}

