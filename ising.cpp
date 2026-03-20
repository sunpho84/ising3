#include <random>
#include <unistd.h>
#include <omp.h>
#include <vector>
#include <iostream>

using namespace std;

//#define PLOT

#include <chrono>

size_t totalTime=0;
size_t computeEnTime=0;

auto now()
{
  return chrono::high_resolution_clock::now();
}

size_t timeFrom(chrono::high_resolution_clock::time_point from)
{
  return chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now()-from).count();
}

int computeEn(vector<int>& conf,int L,int N)
{
  int en=0;
 #pragma omp parallel for reduction(+:en)
	for(int iSite=0;iSite<N;iSite++)
    {
      int y=iSite/L;
      int x=iSite%L;
      
      int neighSiteX=y*L+(x+1)%L;
      int neighSiteY=((y+1)%L)*L+x;
      
      en-=conf[neighSiteX]*conf[iSite]+conf[neighSiteY]*conf[iSite];
    }
  
  return en;
}

int computeSiteEn(vector<int>& conf,int L,int N,int iSite)
{
  int en=0;
      int y=iSite/L;
      int x=iSite%L;
      
      int neighSiteX=y*L+(x+1)%L;
      int neighSiteX2=y*L+(x+L-1)%L;
      int neighSiteY=((y+1)%L)*L+x;
      int neighSiteY2=((y+L-1)%L)*L+x;
      
      en-=conf[neighSiteX]*conf[iSite]+conf[neighSiteY]*conf[iSite];
      en-=conf[neighSiteX2]*conf[iSite]+conf[neighSiteY2]*conf[iSite];
  
  return en;
}

double computeMagnetization(vector<int>& conf,int L,int N)
{
  int mag=0;
  for(int iSite=0;iSite<N;iSite++)
    mag+=conf[iSite];
  
  return (double)mag/N;
}

int main()
{
  const size_t nThreads=omp_get_max_threads();
  printf("NThreads: %zu\n",nThreads);
	
#ifdef PLOT
  FILE* gp=popen("gnuplot","w");
  fprintf(gp,"unset key\n");
  fprintf(gp,"set style fill solid\n");
#endif
  
  double beta=1.4407228;
  int L=40;
  int N=L*L;
  int seed=124634;
  
  vector<int> conf(N);
  
  mt19937 gen(seed);
  
  for(int& c : conf)
    c=binomial_distribution<int>(1,0.5)(gen)*2-1;
  
  int nConfs=10000;
  
  auto beginProgTime=now();
  
  /** Produce nConfs */
  for(int iConf=0;iConf<nConfs;iConf++)
    {
      /** Update each esite*/
      for(int iSite=0;iSite<N;iSite++)
	{
	  // cout<<"Looping on site "<<iSite<<endl; 
	  
	  int backupSiteState=conf[iSite];
	  
	  // cout<<"Before: "<<conf[iSite]<<endl;
	  int enBefore=computeEn(conf,L,N);
	  // cout<<"enBefore: "<<enBefore<<endl;
	  conf[iSite]=-conf[iSite];
	  
	  // cout<<"After: "<<conf[iSite]<<endl;
	  auto beginEnMeas=now();
	  int enAfter=computeEn(conf,L,N);
	  // cout<<"enAfter: "<<enAfter<<endl;
	  computeEnTime+=timeFrom(beginEnMeas);
	  
	  int eDiff=enAfter-enBefore;
	  // cout<<"eDiff: "<<eDiff<<endl;
	  
	  if(eDiff<=0)
	    // cout<<"Accepted as energy is decreasing"<<endl
	    ;
	  else
	    {
	      double pAcc=exp(-beta*eDiff);
	      
	      // cout<<"Pacc: "<<pAcc<<endl;
	      uniform_real_distribution<double> distrAcc(0,1.0);
	      
	      int acc=distrAcc(gen)<pAcc;
	      // cout<<"acc: "<<acc<<endl;
	      
	      if(acc==0)
		{
		  conf[iSite]=backupSiteState;
		  // cout<<"Not accepted"<<endl;
		}
	      // else
	      // 	cout<<"Accepted"<<endl;
	    }
	}
      
#ifdef PLOT
      fprintf(gp,"plot '-' w boxxyerror\n");
      for(int site=0;site<N;site++)
	if(conf[site]==-1)
	  fprintf(gp,"%lg %lg 0.5 0.5\n",site%L+0.5,int(site/L)+0.5);
      fprintf(gp,"e\n");
      fflush(gp);
#endif
      
      double mag=computeMagnetization(conf,L,N);
      cout<<"Mag "<<mag<<endl;
      
      //sleep(1);
    }
  
  totalTime+=timeFrom(beginProgTime);
  
  cout<<"TotalTime: "<<totalTime/1e9<<" s"<<endl;
  cout<<"ComputeEnergyTime: "<<computeEnTime/1e9<<" s"<<endl;
  
#ifdef PLOT
  pclose(gp);
#endif
  
  return 0;
}
