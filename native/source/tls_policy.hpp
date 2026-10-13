#pragma once
#include <cstdint>
// libnx Auto/Auto24 stop at TLS 1.2, even on firmware supporting TLS 1.3.
// Upgrade only the default; leave explicit protocol choices unchanged.
inline uint32_t defaultTlsVersions(uint32_t versions,bool modernFirmware){
 if(versions==1u||versions==(1u<<24))return (1u<<5)|(modernFirmware?(1u<<6):0u);
 return versions;
}
