#ifndef HBASE_HH
#define HBASE_HH

#include <TH2D.h>
#include <vector>
#include "TFile.h"
#include "TTree.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

class HBase{
		public:
				//Constructor, destructor and instance of base class
				HBase();
				virtual ~HBase();

		protected:
				//Protected member functions
				virtual void ReadTree(const TString &fname,const TString &tname); //Read TTree from ROOT files
				virtual void ReadList(const string &_list); // Read the file list and save to the protected vector
				virtual void CreateFile(const TString &_outname); // Create output file
				virtual void Init(const TString &_outname);// Initialize derived members

				// Protected member variables
				vector<string>	list;
				TFile *fin;	//TFile pointer to open files
				TTree *tin;	//Read Tree from fin
				TFile *fout;	//Create output files
				TTree *tout;	//Create output trees
				int   _Run_No;
				int   _cycleID;
				int   _triggerID;
				unsigned int   _Event_Time;
				vector< int > *_cellID;
				vector< int > *_bcid;
				vector< int > *_hitTag;
				vector< int > *_gainTag;
				vector< int > *_cherenkov;
				vector< double > *_HG_Charge;
				vector< double > *_LG_Charge;
				vector< double > *_Hit_Time;

};

#endif
