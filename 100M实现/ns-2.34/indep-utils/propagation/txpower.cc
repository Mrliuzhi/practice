#include <math.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include <cstring>
#ifndef M_PI
#define M_PI 3.14159265359
#endif

double Friis(double Pr,double Gt,double Gr,double lambda,double L,double d)
{
double M=(4*M_PI*d) / lambda;
return (Pr*L*(M*M)) /(Gt*Gr);
}
double TwoRay(double Pr,double Gt,double Gr,double ht,double hr,double L,double d,double lambda)
{
double Pt;
double crossover_dist=(4*M_PI*ht*hr) / lambda;
if(d<crossover_dist)
Pt=Friis(Pr,Gt,Gr,lambda,L,d);
else
Pt=(d*d*d*d*L*Pr)/(Gt*Gr*hr*hr*ht*ht);
return Pt;
}

double inv_erfc(double y)
{
double s,t,u,w,x,z;
z=y;
if(y>1) {
z=2-y;
}
w=0.916461398268964-log(z);
u=sqrt(w);
s=(log(u)+0.488826640273108)/w;
t=1/(u+0.231729200323405);
x=u*(1-s*(s*0.124610454613712+0.5))-((((-0.072884676585675*t+0.269999308670029)*t+0.150689047360223)*t+0.116065025341614)*t+0.499999303439796)*t;
t=3.97886080735226/(x+3.97886080735226);
u=t-0.5;
s=(((((((((0.00112648096188977922*u+1.05739299623423047e-4)*u-0.00351287146129100025)*u-7.71708358954120939e-4)*u+0.00685649426074558612)*u+0.00339721910367775861)*u-0.011274916933250487)*u-0.0118598117047771104)*u+0.0142961988697898018)*u+0.0346494207789099922)*u+0.00220995927012179067;
s=((((((((((((s*u-0.0743424357241784861)*u-0.105872177941595488)*u+0.0147297938331485121)*u+0.316847638520135944)*u+0.713657635868730364)*u+1.05375024970847138)*u+1.21448730779995237)*u+1.16374581931560831)*u+0.956464974744799006)*u+0.686265948274097816)*u+0.434397492331430115)*u+0.244044510593190935)*t-z*exp(x*x-0.120782237635245222);
x+=s*(x*s+1);
if(y>1) {
x=-x;
}
return x;
}
double inv_Q(double y)
{
double x;
x=sqrt(2.0)*inv_erfc(2.0*y);
return x;
}
int main(int argc, char** argv)
{
char** propModel=NULL;
double Pr=3.65262e-10;
double Gt=1.0;
double Gr=1.0;
double freq=914.0e6;
double sysLoss=1.0;
double ht=1.5;
double hr=1.5;
double pathlossExp_=2.0;
double std_db_ =4.0;
double dist0_ =1.0;
double prob =0.95;
double Pt_;
if(argc<4) {
cout <<"USAGE:find transmit power for certain communication range (distance)" << endl;
cout << endl;
cout << "SYNOPSIP:threshold -m <propagation-model> [other-options] distance" << endl;
cout << endl;
cout << "<propagation-model>:FreeSpace,TwoRayGround or Shadowing" << endl;
cout << "[other-options]: set parameters other than default values:" << endl;
cout << endl << "common parameters:" << endl;
cout << "-Pr <receive-threshold>" << endl;
cout << "-fr <frequency>" << endl;
cout << "Gt <transmit-antenna-gain>" << endl;
cout << "Gr <receive-antenna-gain>" << endl;
cout << "-L <system-loss>" << endl;
cout << endl << "For two-ray ground model:" << endl;
cout << "-ht <transmit-antenna-height>" << endl;
cout << "-hr <receive-antenna-height>" << endl;
cout << endl << " For shadowing model:" << endl;
cout << "-pl <path-loss-exponent>" << endl;
cout << "-std <shadowing-deviation>" << endl;
cout << "-d0 <reference-distance>" << endl;
cout << "-r <receiving-rate>" << endl;
return 0;
}
double dist=atof(argv[argc-1]);
cout << "distance = " << dist << endl;
int argCount = (argc-2) / 2;
argv++;
for(int i=0;i<argCount;i++) {
if(!strcmp(*argv,"-m")) {
propModel=argv+1;
cout << "propagation model:" << *propModel << endl;
}
if(!strcmp(*argv,"-Pr")) {
Pr=atof(*(argv+1));
}
if(!strcmp(*argv,"-fr")) {
freq=atof(*(argv+1));
}
if(!strcmp(*argv,"-Gt")) {
Gt=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-Gr")) {
Gr=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-L")) {
sysLoss=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-ht")) {
ht=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-hr")) {
hr=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-pl")) {
pathlossExp_=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-std")) {
std_db_=atof(*(argv+1)); 
}
if(!strcmp(*argv,"-d0")) {
dist0_ =atof(*(argv+1)); 
}
if(!strcmp(*argv,"-r")) {
prob=atof(*(argv+1)); 
}
argv+=2;
}
if(propModel==NULL) {
cout << "Must specify propagation model:-m <propagation model > " << endl;
return 0;
}
double lambda=3.0e8/freq;
if(!strcmp(*propModel,"FreeSpace")) {
Pt_ = Friis(Pr,Gt,Gr,lambda,sysLoss,dist);
cout << endl << "Selected parameters:" << endl;
cout << "receive power threshold:" << Pr << endl;
cout << "frequency: " << freq << endl;
cout << "transmit antenna gain:" << Gt << endl;
cout << "receive antenna gain:" << Gr << endl;
cout << "system loss:" << sysLoss << endl;
} else if (!strcmp(*propModel,"TwoRayGround")) {
Pt_ = TwoRay(Pr,Gt,Gr,ht,hr,sysLoss,dist,lambda);
cout << endl << "Selected parameters:" << endl;
cout << "receive power threshold:" << Pr << endl;
cout << "frequency:" << freq << endl;
cout << "transmit antenna gain:" << Gt << endl;
cout << "receive antenna gain:" << Gr << endl;
cout << "system loss:" << sysLoss << endl;
cout << "transmit antenna height:" << ht << endl;
cout << "receive antenna height:" << hr << endl;
} else if (!strcmp(*propModel, "Shadowing")) {
double avg_db = -10.0*pathlossExp_ * log10(dist/dist0_);
double invq=inv_Q(prob);
double threshdb=invq*std_db_+avg_db;
double Pr0=Pr/pow(10.0,threshdb/10.0);
Pt_=Friis(Pr0,Gt,Gr,lambda,sysLoss,dist0_);
cout << "Pr0=" << Pr0 << endl;
cout << "avg_db=" << avg_db << endl;
cout << "invq=" << invq << endl;
cout << "threshdb=" << threshdb << endl;
cout << endl << "Selected parameters:" << endl;
cout << "receive power threshold:" << Pr << endl;
cout << "frequency:" << freq << endl;
cout << "transmit antenna gain:" << Gt << endl;
cout << "receive antenna gain:" << Gr << endl;
cout << "system loss:" << sysLoss << endl;
cout << "path loss exp:" << pathlossExp_ << endl;
cout << "shadowing deviation:" << std_db_ << endl;
cout << "close-in reference distance:" << dist0_ << endl;
cout << "receiving rate:" << prob << endl;
} else {
cout << "Error: unknown propagation model." << endl;
cout << "Available model: FreeSpace, TwoRayGround, Shadowing" << endl;
return 0;
}
cout << endl << "transmit power Pt_ is : " << Pt_ << endl;
}
