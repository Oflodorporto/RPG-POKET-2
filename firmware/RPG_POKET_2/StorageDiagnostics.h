#pragma once
#include <stdint.h>
namespace storageDiag {
enum class Stage:uint8_t {Boot,Downloaded,Installed};
enum class File:uint8_t {GenericArt,FirmwarePart,ArtPart,VersionedArt};
enum class Issue:uint8_t {None,Mount,Open,Size,Read,Magic,Length,HeaderCrc,Catalog,Crc,Sha,HashApi};
// Fixed RAM evidence only: no file paths, credentials, save bytes or SD/NVS logs.
struct Failure {Stage stage=Stage::Boot;File file=File::GenericArt;Issue issue=Issue::None;uint32_t offset=0,expected=0,actual=0;int code=0;bool valid()const{return issue!=Issue::None;}};
inline void set(Failure* f,Issue issue,uint32_t offset=0,uint32_t expected=0,uint32_t actual=0,int code=0){if(f){f->issue=issue;f->offset=offset;f->expected=expected;f->actual=actual;f->code=code;}}
inline const char* fileName(File f){return f==File::FirmwarePart?"firmware.part":f==File::ArtPart?"artes.part":f==File::VersionedArt?"artes final":"artes.pak";}
inline const char* reason(Issue i){switch(i){case Issue::Mount:return "montar SD";case Issue::Open:return "abrir";case Issue::Size:return "tamanho";case Issue::Read:return "leitura";case Issue::Magic:return "assinatura PKA1";case Issue::Length:return "payload tamanho";case Issue::HeaderCrc:return "CRC cabecalho";case Issue::Catalog:return "catalogo";case Issue::Crc:return "CRC dos dados";case Issue::Sha:return "SHA-256 prefixo";case Issue::HashApi:return "API SHA-256";default:return "nenhuma";}}
inline bool hexValues(Issue i){return i==Issue::Magic||i==Issue::HeaderCrc||i==Issue::Crc||i==Issue::Sha;}
inline Failure bootArtFailure;
}
