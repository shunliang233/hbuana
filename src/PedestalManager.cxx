#include "PedestalManager.h"
#include "TStyle.h"
#include <TH2.h>
#include <TF1.h>
#include <memory>
#include <iostream>
#include <TCanvas.h>
#include <sstream>
#include <algorithm>
#include "TSpectrum.h"

using namespace std;
bool compare(double a, double b){
	return a<b;
}
double maxx(double a, double b){
	return a>b?a:b;
}
double minn(double a, double b){
	return a<b?a:b;
}
PedestalManager *_instance = nullptr;
//Get Instance Class
PedestalManager *PedestalManager::CreateInstance()
{
	if(_instance == nullptr)
	{
		_instance = new PedestalManager();
	}
	return _instance;
}

void PedestalManager::DeleteInstance()
{
	if(_instance == nullptr)
	{

	}
	else
	{
		delete _instance;
	}
}

PedestalManager::PedestalManager()
{
	list.clear();
	cout<<"PedestalManager class instance initialized."<<endl;
}

void PedestalManager::Init(const TString &_outname)
{
	CreateFile(_outname);
	tout = new TTree("pedestal","Pedestal");
	tout->Branch("cellid",&_cellid);
	tout->Branch("highgain_peak",&highgain_peak);
	tout->Branch("highgain_rms",&highgain_rms);
	tout->Branch("lowgain_peak",&lowgain_peak);
	tout->Branch("lowgain_rms",&lowgain_rms);
	highgainpeak=std::make_unique<TH2D>("highgainpeak","HighGain Peak",360,0,360,36,0,36);
	highgainrms=std::make_unique<TH2D>("highgainrms","HighGain RMS",360,0,360,36,0,36);
	lowgainpeak=std::make_unique<TH2D>("lowgainpeak","LowGain Peak",360,0,360,36,0,36);
	lowgainrms=std::make_unique<TH2D>("lowgainrms","LowGain RMS",360,0,360,36,0,36);
	int ini_cellid=0;
	for(int i_layer=0;i_layer<40;i_layer++)
	{
		for(int i_chip=0;i_chip<9;i_chip++)
		{
			for(int i_chn=0;i_chn<36;i_chn++)
			{
				ini_cellid=i_layer*100000+i_chip*10000+i_chn;
				vec_cellid.push_back(ini_cellid);
				TString highgainname="highgain_"+TString(to_string(ini_cellid).c_str());
				map_cellid_highgain[ini_cellid] = new TH1D(highgainname,highgainname,1500,0,1500);
				TString lowgainname="lowgain_"+TString(to_string(ini_cellid).c_str());
				map_cellid_lowgain[ini_cellid] = new TH1D(lowgainname,lowgainname,1600,0,1600);
			}
		}
	}
	for(int i=0;i<40;i++)
	{
		TString name_highgainpeak="highgainpeak_"+TString(to_string(i).c_str());
		map_layer_highgainpeak[i] = new TH2D(name_highgainpeak,name_highgainpeak,9,0,9,36,0,36);
		TString name_highgainrms="highgainrms_"+TString(to_string(i).c_str());
		map_layer_highgainrms[i] = new TH2D(name_highgainrms,name_highgainrms,9,0,9,36,0,36);
		TString name_lowgainpeak="lowgainpeak_"+TString(to_string(i).c_str());
		map_layer_lowgainpeak[i] = new TH2D(name_lowgainpeak,name_lowgainpeak,9,0,9,36,0,36);
		TString name_lowgainrms="lowgainrms_"+TString(to_string(i).c_str());
		map_layer_lowgainrms[i] = new TH2D(name_lowgainrms,name_lowgainrms,9,0,9,36,0,36);
	}
	cout<<"Initialization done"<<endl;
}

int PedestalManager::AnaPedestal(const std::string &_list,const int &sel_hittag)
{
	cout<<"Starting Ana"<<endl;
	cout<<"Ana preparation done"<<endl;
	ReadList(_list); // read file list _list to list
	cout<<"read list done"<<endl;
	cout<<usemt<<" usemt"<<endl;
	if(usemt){
		ROOT::EnableImplicitMT();
		ROOT::EnableThreadSafety();
		const int nthreads = 10;
		thread t[nthreads];
		vector<vector<string>> listth;
		int nfile_pert = list.size()/nthreads +1;
		if(list.size()%nthreads == 0)nfile_pert--;
		auto f = [this,sel_hittag](vector<string> list_tmp)
		{
			for(auto tmp:list_tmp)
			{
				string skipchannel = tmp;
				int dac_chn=-1;// Which channel should not be used here for pedestal analysis
				if(sel_hittag == 1)
				{
					skipchannel = skipchannel.substr(skipchannel.find_last_of('/')+1);
					skipchannel = skipchannel.substr(skipchannel.find("chn")+3);
					skipchannel = skipchannel.substr(0,skipchannel.find_last_of('_'));
						dac_chn = stoi(skipchannel);
				}
				this->ReadTree(TString(tmp.c_str()),"Raw_Hit");
				int Nentry = tin->GetEntries();
				for(int ientry=0;ientry<Nentry;ientry++)
				{
					tin->GetEntry(ientry);
					for(int i=0;i<_hitTag->size();i++)
					{
						if(_hitTag->at(i)!=sel_hittag)continue;
						int cellid = _cellID->at(i);
						int channel = cellid%100;
						int memo = (cellid%10000)/100;
						if(memo !=0 )continue;
						if(dac_chn==channel)continue;
						int layer = cellid/1e5;
						mtx.lock();
						map_cellid_highgain[cellid]->Fill(_HG_Charge->at(i));
						map_cellid_lowgain[cellid]->Fill(_LG_Charge->at(i));
						mtx.unlock();
					}
				}
			}
		};
		for(int ith=0;ith<nthreads;ith++)
		{
			int begin = ith*nfile_pert;
			int end = begin + nfile_pert - 1;
			end = end>list.size()-1? list.size()-1 : end;
			vector<string> tmp_list;
			for(int i=begin;i<=end;i++)tmp_list.push_back(list.at(i));
			listth.push_back(tmp_list);
		}
		for(int ith=0;ith<nthreads;ith++)
		{
			async(launch::async,f,listth.at(ith));
			//t[ith] = thread(f,listth.at(ith));
		}
		//for(int i=0;i<nthreads;i++)
		//{
		//	if(t[i].joinable())t[i].join();
		//}
	}
	else
	{
		for_each(list.begin(),list.end(),[this,sel_hittag](string tmp){
				string skipchannel = tmp;
				int dac_chn=-1;// Which channel should not be used here for pedestal analysis
				if(sel_hittag == 1){
					skipchannel = skipchannel.substr(skipchannel.find_last_of('/')+1);
					int n_chn=skipchannel.find("chn");
					if(n_chn!=-1){
						skipchannel = skipchannel.substr(n_chn+3);
						skipchannel = skipchannel.substr(0,skipchannel.find_last_of('_'));
						dac_chn = stoi(skipchannel);
					}
				}
				this->ReadTree(TString(tmp.c_str()),"Raw_Hit");
				int Nentry = tin->GetEntries();
				int flag[9][40]={0};
				tin->GetEntry(Nentry-1);
				if(_Event_Time<=0)tin->GetEntry(Nentry-2);
				if(_Event_Time<=0)tin->GetEntry(Nentry-3);
				TH1I *Event_Time = new TH1I("Event_Time","Event_Time",_Event_Time,0,_Event_Time);
				for(int i=0;i<Nentry;i++){
				tin->GetEntry(i);
				Event_Time->Fill(_Event_Time);
				}
				for(int ientry=0;ientry<Nentry;ientry++){
					tin->GetEntry(ientry);
					if(Event_Time->GetBinContent(_Event_Time)<10){
						for(int j=0;j<9;j++)
							for(int p=0;p<40;p++)
								flag[j][p]=0;
						continue;
					}
					for(int i=0;i<_hitTag->size();i++){
						if(_hitTag->at(i)!=sel_hittag)continue;
						int cellid = _cellID->at(i);
						int channel = cellid%100;
						int memo = (cellid%10000)/100;
						if(memo !=0 )continue;
						if(dac_chn==channel)continue;
						int layer = cellid/1e5;
						int chip = (cellid%100000)/10000;
						// if(times->at(i)<time_min)time_min=times->at(i);
						// if(times->at(i)>time_max)time_max=times->at(i);
						// if(lowgains->at(i)<lowgain_min)lowgain_min=lowgains->at(i);
						// if(lowgains->at(i)>lowgain_max)lowgain_max=lowgains->at(i);
						// if(Event_Time->GetBinContent(_Event_Time-1)<10||_Event_Time==0)
						flag[chip][layer]+=1;
						if(flag[chip][layer]>36){
							if(_HG_Charge->at(i)>100)map_cellid_highgain[cellid]->Fill(_HG_Charge->at(i));
							if(_LG_Charge->at(i)>100)map_cellid_lowgain[cellid]->Fill(_LG_Charge->at(i));
						}
					}
				}
		}
		);
	}
	// Analysis done
	//
	// Fill the output tree
	for_each(vec_cellid.begin(),vec_cellid.end(),
			[this](int i)->void{
			_cellid = i ;
			highgain_peak=map_cellid_highgain[i]->GetBinCenter(map_cellid_highgain[i]->GetMaximumBin());
			highgain_rms=map_cellid_highgain[i]->GetRMS();
			double gap = 3*highgain_rms;
			TF1 *f1=new TF1("f1","gaus");
			TSpectrum *s;
			s= new TSpectrum(4);
			int npeaks = s->Search(map_cellid_highgain[i],maxx(1,highgain_rms/4),"nobackground",0.2);
			double *xpeaks = s->GetPositionX();
			sort(xpeaks,xpeaks+npeaks,compare);
			if(npeaks>1){
			gap=xpeaks[1]-xpeaks[0];
			for(int p=1;p<npeaks-1;p++){
			gap=minn(gap , xpeaks[p+1]-xpeaks[p]);
			}
			}
			highgain_rms=minn(0.5*gap , 1.5*maxx(highgain_rms,2));
			// highgain_rms=(highgain_rms>2)?highgain_rms:2;
			// highgain_rms=(highgain_rms<5)?highgain_rms:5;
			for(int n=0;n<4;n++){
				map_cellid_highgain[i]->Fit(f1,"q","",highgain_peak-highgain_rms,highgain_peak+highgain_rms);
				// highgain_peak=f1->GetParameter(1);
				highgain_rms=f1->GetParameter(2);
				highgain_rms=minn(0.5*gap , 1.5*maxx(highgain_rms,2));
				// highgain_rms=(highgain_rms>2)?highgain_rms:2;
				// highgain_rms=(highgain_rms<5)?highgain_rms:5;
			}
			highgain_peak=f1->GetParameter(1);
			highgain_rms=f1->GetParameter(2);
			// if(highgain_rms>4.5){
			// highgain_peak=map_cellid_highgain[i]->GetBinCenter(map_cellid_highgain[i]->GetMaximumBin());
			// highgain_rms=map_cellid_highgain[i]->GetRMS();
			// highgain_rms=(highgain_rms>2)?highgain_rms:2;
			// highgain_rms=(highgain_rms<5)?highgain_rms:5;
			// for(int n=0;n<3;n++){
			// map_cellid_highgain[i]->Fit(f1,"q","",highgain_peak-0.8*highgain_rms,highgain_peak+0.8*highgain_rms);
			// highgain_peak=f1->GetParameter(1);
			// highgain_rms=f1->GetParameter(2);
			// highgain_rms=(highgain_rms>2)?highgain_rms:2;
			// highgain_rms=(highgain_rms<5)?highgain_rms:5;
			// }
			// }
			// highgain_rms=f1->GetParameter(2);

			lowgain_peak=map_cellid_lowgain[i]->GetBinCenter(map_cellid_lowgain[i]->GetMaximumBin());
			lowgain_rms=map_cellid_lowgain[i]->GetRMS();
			gap=3*lowgain_rms;
			npeaks = s->Search(map_cellid_lowgain[i],maxx(1,lowgain_rms/4),"nobackground",0.2);
			xpeaks = s->GetPositionX();
			sort(xpeaks,xpeaks+npeaks,compare);
			if(npeaks>1){
				gap=xpeaks[1]-xpeaks[0];
				for(int p=1;p<npeaks-1;p++){
					gap=minn(gap,xpeaks[p+1]-xpeaks[p]);
				}
			}
			lowgain_rms=minn(0.5*gap,1.5*maxx(lowgain_rms,2));
			// lowgain_rms=(lowgain_rms>2)?lowgain_rms:2;
			// lowgain_rms=(lowgain_rms<5)?lowgain_rms:5;
			for(int n=0;n<4;n++){
				map_cellid_lowgain[i]->Fit(f1,"q","",lowgain_peak-lowgain_rms,lowgain_peak+lowgain_rms);
				// lowgain_peak=f1->GetParameter(1);
				lowgain_rms=f1->GetParameter(2);
				lowgain_rms=minn(0.5*gap,1.5*maxx(lowgain_rms,2));
				// lowgain_rms=(lowgain_rms>2)?lowgain_rms:2;
				// lowgain_rms=(lowgain_rms<5)?lowgain_rms:5;
			}
			lowgain_peak=f1->GetParameter(1);
			lowgain_rms=f1->GetParameter(2);
			tout->Fill();
			});
	cout<<"Out Tree Filled"<<endl;
	fout->cd();
	tout->Write();
	// cout<<"time min: "<<time_min<<" max: "<<time_max<<endl;
	// cout<<"lowgain min: "<<lowgain_min<<" max: "<<lowgain_max<<endl;

	// Save hists into the output file:
	// mode_name = times or lowgains
	// tmp_map is the map from cellid to TH1D to loop over
	// tmp_layer_timepeak and tmp_layer_timerms is the map from layer to peak and rms
	// hpeak and hrms are the general TH2D for peak and rms
	// alias is the alternative name. high or low
	auto f_save = [this](TString mode_name,unordered_map<int,TH1D*> tmp_map,unordered_map<int,TH2D*> tmp_layer_gainpeak,unordered_map<int,TH2D*> tmp_layer_gainrms,std::unique_ptr<TH2D> &hpeak,std::unique_ptr<TH2D> &hrms,TString alias)
	{
		fout->mkdir(TString(mode_name));
		fout->cd(TString(mode_name));
		for(int i=0;i<40;i++)gDirectory->mkdir(TString("layer_")+TString(to_string(i).c_str()));
		for(auto i:tmp_map)
		{
			int cellid=i.first;
			int layer = cellid/1e5;
			int channel = cellid%100;
			int chip = (cellid%100000)/10000;
			double ppeak = i.second->GetBinCenter(i.second->GetMaximumBin());
			double rrms = i.second->GetRMS();
			double gap =3*rrms;
			TF1 *f1=new TF1("f1","gaus");
			TSpectrum *s = new TSpectrum(4);
			int npeaks = s->Search(i.second,maxx(1,rrms/4),"nobackground",0.2);
			double *xpeaks = s->GetPositionX();
			sort(xpeaks,xpeaks+npeaks,compare);
			if(npeaks>1){
				gap=xpeaks[1]-xpeaks[0];
				for(int p=1;p<npeaks-1;p++){
					gap=minn(gap,xpeaks[p+1]-xpeaks[p]);
				}
			}
			rrms=minn(0.5*gap,1.5*maxx(rrms,2));
			// rrms=(rrms>2)?rrms:2;
			// rrms=(rrms<5)?rrms:5;
			for(int n=0;n<4;n++){
				i.second->Fit(f1,"q","",ppeak-rrms,ppeak+rrms);
				// ppeak=f1->GetParameter(1);
				rrms=f1->GetParameter(2);
				rrms=minn(0.5*gap,1.5*maxx(rrms,2));
				// rrms=(rrms>2)?rrms:2;
				// rrms=(rrms<5)?rrms:5;
			}
			ppeak=f1->GetParameter(1);
			rrms=f1->GetParameter(2);
			// if(rrms>4.5){
			// ppeak = i.second->GetBinCenter(i.second->GetMaximumBin());
			// rrms = i.second->GetRMS();
			// rrms=(rrms>2)?rrms:2;
			// rrms=(rrms<5)?rrms:5;
			// for(int n=0;n<3;n++){
			// i.second->Fit(f1,"q","",ppeak-0.8*rrms,ppeak+0.8*rrms);
			// ppeak=f1->GetParameter(1);
			// rrms=f1->GetParameter(2);
			// rrms=(rrms>2)?rrms:2;
			// rrms=(rrms<5)?rrms:5;
			// }
			// }
			// if(cellid==140008)printf("%f",ppeak);
			tmp_layer_gainpeak[layer]->Fill(chip,channel,ppeak);
			tmp_layer_gainrms[layer]->Fill(chip,channel,rrms);      
			hpeak->Fill(layer*9+chip,channel,ppeak);
			hrms->Fill(layer*9+chip,channel,rrms);
			TString dir_name = TString(mode_name+"/layer_") + TString(to_string(layer).c_str());
			fout->cd(dir_name);
			i.second->Write();
		}
		for(int i=0;i<40;i++)
		{
			TString dir_name = TString(mode_name+"/layer_") + TString(to_string(i).c_str());
			fout->cd(dir_name);
			tmp_layer_gainpeak[i]->Write();
			tmp_layer_gainrms[i]->Write();
			//this->SaveCanvas(tmp_layer_timepeak[i],alias+TString("gain_peak_")+to_string(i).c_str());
			//this->SaveCanvas(tmp_layer_timerms[i],alias+TString("gain_rms_")+to_string(i).c_str());
		}
		cout<<mode_name<<" done"<<endl;
	};
	f_save("highgain",map_cellid_highgain,map_layer_highgainpeak,map_layer_highgainrms,highgainpeak,highgainrms,"high");
	f_save("lowgain",map_cellid_lowgain,map_layer_lowgainpeak,map_layer_lowgainrms,lowgainpeak,lowgainrms,"low");

	fout->cd("");
	highgainpeak->Write();
	highgainrms->Write();
	lowgainpeak->Write();
	lowgainrms->Write();
	//fout->Close();
	cout<<"2D hists written"<<endl;
	return 0;
}

void PedestalManager::SaveCanvas(TH2D* h,const TString &name)
{
	gStyle->SetPaintTextFormat("4.1f");
	std::unique_ptr<TCanvas> c1=std::make_unique<TCanvas>(TString(name),TString(name),1024,768);
	c1->cd();
	gStyle->SetOptStat("");
	h->Draw("colztext");
	TString outname=name+".png";
	c1->SaveAs(TString(outname));
}

PedestalManager::~PedestalManager()
{
	cout<<"Pedestal destructor called"<<endl;
}
