#include <switch.h>
#include "tls_policy.hpp"
#include "tls_compat.hpp"
#include "diagnostics.hpp"
namespace {
thread_local std::string tlsError;
void recordTlsError(const char* stage,Result result){
 char code[96];snprintf(code,sizeof(code),"SSL %s failed: 0x%X (module=%u, description=%u)",stage,R_VALUE(result),R_MODULE(result),R_DESCRIPTION(result));
 tlsError=code;diagnostic("ssl",tlsError);
}
}
void resetTlsError(){tlsError.clear();}
std::string lastTlsError(){return tlsError;}
extern "C" Result __real_sslCreateContext(SslContext*,u32);
extern "C" Result __real_sslConnectionDoHandshake(SslConnection*,u32*,u32*,void*,u32);
// The packaged curl backend cannot request TLS 1.3. Intercept its default
// context creation without changing certificate or hostname verification.
extern "C" Result __wrap_sslCreateContext(SslContext* context,u32 versions){
 versions=defaultTlsVersions(versions,hosversionAtLeast(11,0,0));
 Result rc=__real_sslCreateContext(context,versions);
 if(R_FAILED(rc))recordTlsError("context",rc);
 return rc;
}
extern "C" Result __wrap_sslConnectionDoHandshake(SslConnection* connection,u32* size,u32* count,void* buffer,u32 capacity){
 Result rc=__real_sslConnectionDoHandshake(connection,size,count,buffer,capacity);
 if(R_FAILED(rc)&&R_VALUE(rc)!=MAKERESULT(123,204))recordTlsError("handshake",rc);
 return rc;
}
