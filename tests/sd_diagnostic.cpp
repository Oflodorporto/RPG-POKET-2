#include <assert.h>
#include <errno.h>
#include <fstream>
#include <vector>
#include <string>
#include <openssl/evp.h>
#include "../tools/sd_diagnostic/SD_Diagnostico/DiagnosticCore.h"
#include "../tools/sd_diagnostic/SD_Diagnostico/Fixture.h"
struct Hash{EVP_MD_CTX* c=EVP_MD_CTX_new();~Hash(){EVP_MD_CTX_free(c);}bool begin(){return EVP_DigestInit_ex(c,EVP_sha256(),nullptr)==1;}bool update(const uint8_t* b,size_t n){return EVP_DigestUpdate(c,b,n)==1;}bool finish(char* out){uint8_t b[32];unsigned n=0;if(EVP_DigestFinal_ex(c,b,&n)!=1||n!=32)return false;for(unsigned i=0;i<32;++i)snprintf(out+2*i,3,"%02x",b[i]);return true;}};
struct Reader{std::vector<uint8_t>& b;size_t pos=0,calls=0,failAt=SIZE_MAX,returnBytes=0;int error=0;bool changedSize=false;uint32_t size(){return b.size()-(changedSize?1:0);}diag::Read read(uint8_t* out,size_t n){++calls;size_t got=std::min(n,b.size()-pos);int e=0;if(pos==failAt){got=std::min(got,returnBytes);e=error;}memcpy(out,b.data()+pos,got);pos+=got;return {got,e};}};
std::vector<uint8_t> load(const char* p){std::ifstream f(p,std::ios::binary);assert(f);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char** argv){assert(argc==3);auto art=load(argv[1]),fixtureData=load(argv[2]);uint8_t buf[4096];auto progress=[](uint32_t){return true;};unsigned cases=0;
 for(unsigned fault=0;fault<7;++fault){auto bytes=art;Reader r{bytes};if(fault==1){r.failAt=diag::TAIL;r.returnBytes=1008;r.error=EIO;}if(fault==2){r.failAt=diag::TAIL;r.returnBytes=1008;}if(fault==3){r.failAt=16;r.returnBytes=0;r.error=EIO;}if(fault==4)r.changedSize=true;if(fault==5)bytes.back()^=1;if(fault==6)bytes[0]^=1;Hash h;auto out=diag::verify(r,h,buf,progress);assert(out.ok==(fault==0));if(fault==1||fault==2){assert(out.offset==6660112&&out.requested==1344&&out.received==1008&&out.error==(fault==1?EIO:0));assert(r.pos==6661120&&r.calls==1628);}++cases;}
 {Reader r{art};Hash h;auto out=diag::verify(r,h,buf,[](uint32_t p){return p<16;});assert(!out.ok&&!strcmp(out.cause,"time-limit")&&r.calls==1);++cases;}
 {Reader r{fixtureData};Hash h;auto out=diag::verify(r,h,buf,progress,diag::FILE_BYTES,fixture::SHA,false);assert(out.ok);++cases;}
 {auto prefix=std::vector<uint8_t>(fixtureData.begin(),fixtureData.begin()+diag::CHECKPOINT);Hash ph;char expected[65];ph.begin();ph.update(prefix.data(),prefix.size());ph.finish(expected);Reader r{prefix};Hash h;assert(diag::verify(r,h,buf,progress,diag::CHECKPOINT,expected,false).ok);prefix[20]^=1;Reader bad{prefix};Hash h2;assert(!diag::verify(bad,h2,buf,progress,diag::CHECKPOINT,expected,false).ok);++cases;}
 assert(diag::exactWrite(4096,4096,0));assert(!diag::exactWrite(4096,4095,0));assert(!diag::exactWrite(4096,4096,EIO));assert(!diag::exactWrite(4096,size_t(-1),EIO));++cases;
 diag::Session s;assert(s.take('4')&&!s.take('1')&&!s.take('4'));assert(s.take('N')&&!s.take('N'));assert(s.take('D')&&!s.take('D'));assert(s.take('S')&&!s.take('S'));assert(!s.take('R'));++cases;
 assert(diag::safeHttps("https://raw.githubusercontent.com/Oflodorporto/RPG-POKET-2/sha/file"));assert(!diag::safeHttps("https://raw.githubusercontent.com.attacker.example/file"));assert(!diag::safeHttps("http://raw.githubusercontent.com/file"));assert(!diag::safeHttps("https://raw.githubusercontent.com@evil/file"));++cases;
 struct Canary{unsigned before=0x12345678;diag::Report r;unsigned after=0xabcdef12;} c;std::string longText(diag::LOG_CAP+100,'x');c.r.append(longText.c_str());c.r.append("end");assert(c.before==0x12345678&&c.after==0xabcdef12&&c.r.truncated&&c.r.used==diag::LOG_CAP-1&&c.r.data[c.r.used]==0);++cases;
 {char secret[5]={};diag::PrivateInput input{secret,sizeof(secret)};for(char c:std::string("abcd"))assert(input.feed(c)==diag::InputResult::Pending);assert(input.feed('\n')==diag::InputResult::Done&&!strcmp(secret,"abcd"));char small[2]={};diag::PrivateInput overflow{small,sizeof(small)};assert(overflow.feed('a')==diag::InputResult::Pending);assert(overflow.feed('S')==diag::InputResult::Invalid&&small[0]==0&&small[1]==0);assert(overflow.feed('\n')==diag::InputResult::Invalid);for(char late:std::string("SND41"))assert(!diag::commandAllowed(true,late));assert(diag::commandAllowed(true,'R')&&diag::commandAllowed(true,'H'));++cases;}
 printf("PASS: %u diagnostic cases: real SHA, exact physical tail fault, no retry, truncation/header/hash, prefix readback, short-write policy, one-shot commands, HTTPS allowlist, bounded report canaries.\n",cases);
}
