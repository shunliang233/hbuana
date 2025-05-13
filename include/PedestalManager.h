#ifndef PEDESTALMANAGER_HH
#define PEDESTALMANAGER_HH

#include "HBase.h"
#include <TH2D.h>
#include <vector>
#include <map>
#include <unordered_map>
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TROOT.h"
#include <fstream>
#include <string>
#include <atomic>
#include <mutex>
#include <future>
#include <thread>
#include <utility>
#include <chrono>

using namespace std;

class PedestalManager : public HBase{

public:
	static PedestalManager* CreateInstance();
	static void DeleteInstance();
	~PedestalManager();

private:
	PedestalManager();

public:
	//Delete Copy constructor
	PedestalManager(const PedestalManager &) = delete;
	PedestalManager &operator=(PedestalManager const &) = delete;

	void Init(const TString &_outname);
	int AnaPedestal(const std::string &list,const int &sel_hittag);
	void Setmt(bool mt){usemt = mt;};
	
private:
	//using HBase::HBase;
	mutex mtx;
	bool usemt=0;
	vector<int> vec_cellid;
	std::unique_ptr<TH2D> highgainpeak;
	std::unique_ptr<TH2D> highgainrms;
	std::unique_ptr<TH2D> lowgainpeak;
	std::unique_ptr<TH2D> lowgainrms;
	unordered_map<int,TH2D*> map_layer_lowgainpeak;
	unordered_map<int,TH2D*> map_layer_lowgainrms;
	unordered_map<int,TH2D*> map_layer_highgainpeak;
	unordered_map<int,TH2D*> map_layer_highgainrms;
	unordered_map<int,TH1D*> map_cellid_highgain;
	unordered_map<int,TH1D*> map_cellid_lowgain;
	double lowgain_min=1000.,lowgain_max=0.;
	double highgain_min=1000.,highgain_max=0.;
	//Branch Name
	double highgain_peak=0.,highgain_rms=0.,lowgain_peak=0.,lowgain_rms=0.;
	int _cellid;
	
	void SaveCanvas(TH2D* h,const TString &name);
};

extern PedestalManager *_instance;

#endif
