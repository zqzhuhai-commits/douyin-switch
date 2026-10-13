// C++ port of Evil0ctal/Douyin_TikTok_Download_API native signing algorithms.
// Modified for Switch: SM3, RC4, A-Bogus and web signature generation in C++.
// Upstream Apache-2.0 attribution and license are in third_party/.
#pragma once
#include <array>
#include <vector>
#include <string>
#include <functional>
#include <random>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <cctype>
#ifdef __SWITCH__
#include <switch.h>
#include <mbedtls/md.h>
#else
#include <openssl/evp.h>
#endif
namespace DouyinSign {
using Bytes=std::vector<uint8_t>;
inline uint32_t rot(uint32_t v,unsigned n){n%=32;return n?(v<<n)|(v>>(32-n)):v;}
inline Bytes sm3(Bytes bytes){
 uint64_t bits=uint64_t(bytes.size())*8;bytes.push_back(128);
 while(bytes.size()%64!=56)bytes.push_back(0);
 for(int i=7;i>=0;i--)bytes.push_back(bits>>(i*8));
 std::array<uint32_t,8> h={0x7380166F,0x4914B2B9,0x172442D7,0xDA8A0600,0xA96F30BC,0x163138AA,0xE38DEE4D,0xB0FB0E4E};
 for(size_t off=0;off<bytes.size();off+=64){
  uint32_t w[68],wp[64];
  for(int i=0;i<16;i++)w[i]=(uint32_t(bytes[off+i*4])<<24)|(uint32_t(bytes[off+i*4+1])<<16)|(uint32_t(bytes[off+i*4+2])<<8)|bytes[off+i*4+3];
  for(int i=16;i<68;i++){uint32_t x=w[i-16]^w[i-9]^rot(w[i-3],15);w[i]=x^rot(x,15)^rot(x,23)^rot(w[i-13],7)^w[i-6];}
  for(int i=0;i<64;i++)wp[i]=w[i]^w[i+4];
  auto v=h;
  for(int i=0;i<64;i++){
   uint32_t ss1=rot(rot(v[0],12)+v[4]+rot(i<16?0x79CC4519:0x7A879D8A,i),7),ss2=ss1^rot(v[0],12);
   uint32_t ff=i<16?v[0]^v[1]^v[2]:(v[0]&v[1])|(v[0]&v[2])|(v[1]&v[2]);
   uint32_t gg=i<16?v[4]^v[5]^v[6]:(v[4]&v[5])|(~v[4]&v[6]);
   uint32_t tt1=ff+v[3]+ss2+wp[i],tt2=gg+v[7]+ss1+w[i];
   v[3]=v[2];v[2]=rot(v[1],9);v[1]=v[0];v[0]=tt1;
   v[7]=v[6];v[6]=rot(v[5],19);v[5]=v[4];v[4]=tt2^rot(tt2,9)^rot(tt2,17);
  }
  for(int i=0;i<8;i++)h[i]^=v[i];
 }
 Bytes out;for(auto x:h)for(int i=3;i>=0;i--)out.push_back(x>>(i*8));return out;
}
inline Bytes raw(const std::string&s){return {s.begin(),s.end()};}
inline Bytes rc4(const Bytes&key,const Bytes&data){
 if(key.empty())throw std::runtime_error("empty RC4 key");
 std::array<unsigned,256> box;for(unsigned i=0;i<256;i++)box[i]=255-i;
 unsigned j=0;for(unsigned i=0;i<256;i++){j=(j*box[i]+j+key[i%key.size()])%256;std::swap(box[i],box[j]);}
 Bytes out;unsigned i=0;j=0;
 for(auto b:data){i=(i+1)%256;j=(j+box[i])%256;std::swap(box[i],box[j]);out.push_back(b^box[(box[i]+box[j])%256]);}return out;
}
inline std::string b64(const Bytes&data,const std::string&alpha){
 std::string out;
 for(size_t i=0;i<data.size();i+=3){size_t n=std::min<size_t>(3,data.size()-i);uint32_t block=uint32_t(data[i])<<16;if(n>1)block|=uint32_t(data[i+1])<<8;if(n>2)block|=data[i+2];
  for(size_t j=0;j<n+1;j++)out+=alpha[(block>>(18-j*6))&63];}
 while(out.size()%4){out+='=';}return out;
}
inline std::string encode(const std::string&s){const char*hex="0123456789ABCDEF";std::string out;for(unsigned char c:s){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='*'||c=='-'||c=='.'||c=='_'||c=='~')out+=c;else{out+='%';out+=hex[c>>4];out+=hex[c&15];}}return out;}
inline std::string hex(const Bytes&b){const char*h="0123456789abcdef";std::string out;for(auto x:b){out+=h[x>>4];out+=h[x&15];}return out;}
inline std::string md5(const std::string&s){Bytes out(16);
#ifdef __SWITCH__
 if(mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_MD5),reinterpret_cast<const unsigned char*>(s.data()),s.size(),out.data())!=0)throw std::runtime_error("MD5 failed");
#else
 unsigned size=16;if(EVP_Digest(s.data(),s.size(),out.data(),&size,EVP_md5(),nullptr)!=1)throw std::runtime_error("MD5 failed");
#endif
 return hex(out);
}
using Pairs=std::vector<std::pair<std::string,std::string>>;
inline std::string query(const Pairs&p){std::string out;for(auto&x:p){if(!out.empty())out+='&';out+=encode(x.first)+'='+encode(x.second);}return out;}
inline std::string webSignature(const std::string&covered,const std::string&uifid,int64_t seconds){return md5(uifid+"_"+std::to_string(seconds)+"_A96D855A08C0A9707F8BEF0D9A527E4E_"+covered);}
inline std::string aBogus(const std::string&q,const std::string&ua,int64_t now,std::function<double()>rng,const std::string&geometry="1920|947|1920|1032|1920|1032|1920|1080|Win32"){
 constexpr int64_t epoch=1721836800000LL;if(now<epoch)throw std::runtime_error("系统时间不正确，请同步时间");
 for(unsigned char c:ua+geometry)if(c>=128)throw std::runtime_error("unsupported fingerprint encoding");
 auto draw=[&](double n){return int(rng()*n);};
 auto mask=[&](int a,int b,int low=-1,int high=-1){int noise=draw(65535);if(low<0)low=noise&255;if(high<0)high=(noise>>8)&255;return Bytes{uint8_t((low&170)|(a&85)),uint8_t((low&85)|(a&170)),uint8_t((high&170)|(b&85)),uint8_t((high&85)|(b&170))};};
 auto digest=[&](const std::string&s){return sm3(sm3(raw(s+"dhzx")));};
 auto dq=digest(q),db=digest("");
 const std::string alpha3="ckdp1h4ZKsUB80/Mfvw36XIgR25+WQAlEi7NLboqYTOPuzmFjJnryx9HVGDaStCe",alpha4="Dkdpgh2ZmsQB80/MfvV36XI1R45-WUAlEixNLwoqYTOPuzKFjJnry79HbGcaStCe";
 auto du=sm3(raw(b64(rc4({0,1,14},raw(ua)),alpha3)));
 std::array<uint8_t,86> f={};
 std::string tail=std::to_string((now+3)&255)+",";
 f[24]=41;f[26]=(now-epoch)/(1000LL*60*60*24*14);f[27]=6;f[28]=3;f[35]=1;f[38]=33;f[66]=3;
 f[79]=geometry.size()&255;f[80]=geometry.size()>>8;f[84]=tail.size()&255;f[85]=tail.size()>>8;
 auto little=[&](uint64_t v,int first,int n){for(int i=0;i<n;i++)f[first+i]=v>>(8*i);};
 little(now,29,6);little(14,44,4);little(now-1,60,6);little(6241,67,4);little(6383,71,4);
 auto canary=[](const Bytes&d,int offset,int sentinel,int fallback){for(size_t i=offset;i<d.size();i++)if(d[i]!=sentinel)return int(d[i]);return fallback;};
 f[48]=dq[9];f[49]=dq[18];f[51]=canary(dq,3,11,12);f[52]=db[10];f[53]=db[19];f[55]=canary(db,4,8,9);f[56]=du[11];f[57]=du[21];f[59]=canary(du,5,12,13);
 const int order[]={34,44,56,61,73,29,70,45,35,49,38,66,51,68,28,48,64,47,30,71,26,55,31,69,59,40,62,63,27,72,41,74,57,52,42,39,33,67,53,43,65,46,36,24,60,32,79,80,84,85};
 auto version=mask(1,0);int probe=draw(240);if(probe>109)probe+=probe%2+1;int trip=(draw(255)&77)|178;auto second=mask(1,0,probe,trip);version.insert(version.end(),second.begin(),second.end());
 uint8_t check=0;for(auto x:version)check^=x;Bytes body;for(int index:order){check^=f[index];body.push_back(f[index]);}
 body.insert(body.end(),geometry.begin(),geometry.end());body.insert(body.end(),tail.begin(),tail.end());body.push_back(check);
 auto lower=ua;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return std::tolower(c);});
 int family=210;if(lower.find("edg")!=std::string::npos)family=125;else if(lower.find("huawei")!=std::string::npos)family=170;else if(lower.find("firefox")!=std::string::npos)family=40;else if(lower.find("chrome")!=std::string::npos)family=0;else if(lower.find("safari")!=std::string::npos)family=81;
 int headerNoise=(family+draw(40))&255;auto header=mask(3,82,-1,headerNoise);
 Bytes frame;for(size_t i=0;i<body.size();i+=3){size_t n=std::min<size_t>(3,body.size()-i);if(n<3){frame.push_back(body[i]);if(n>1&&body[i+1])frame.push_back(body[i+1]);continue;}int noise=draw(1000)&255;
  frame.push_back((noise&145)|(body[i]&110));frame.push_back((noise&66)|(body[i+1]&189));frame.push_back((noise&44)|(body[i+2]&211));frame.push_back((body[i]&145)|(body[i+1]&66)|(body[i+2]&44));}
 version.insert(version.end(),frame.begin(),frame.end());auto sealed=rc4({211},version);header.insert(header.end(),sealed.begin(),sealed.end());return b64(header,alpha4);
}
inline double noise(){
#ifdef __SWITCH__
 return (randomGet64()>>11)*(1.0/9007199254740992.0);
#else
 thread_local std::mt19937 gen(std::random_device{}());return std::generate_canonical<double,32>(gen);
#endif
}
}
