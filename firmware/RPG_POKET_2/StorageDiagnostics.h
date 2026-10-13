#pragma once
#include <stdint.h>
namespace storageDiag {
enum class Stage:uint8_t {Boot,Downloaded,Installed,Identity,Installing,Promotion,Preparing};
enum class File:uint8_t {GenericArt,FirmwarePart,ArtPart,VersionedArt,BackupArt,FirmwareTag,ArtTag,Card};
enum class Issue:uint8_t {None,Mount,Open,Size,Read,Magic,Length,HeaderCrc,Catalog,Crc,Sha,HashApi,Rename,BackupExists};
// Fixed RAM evidence only: no file paths, credentials, save bytes or SD/NVS logs.
struct Failure {Stage stage=Stage::Boot;File file=File::GenericArt;Issue issue=Issue::None;uint32_t offset=0,expected=0,actual=0;int code=0;bool valid()const{return issue!=Issue::None;}};
inline void set(Failure* f,Issue issue,uint32_t offset=0,uint32_t expected=0,uint32_t actual=0,int code=0){if(f){f->issue=issue;f->offset=offset;f->expected=expected;f->actual=actual;f->code=code;}}
inline const char* fileName(File f){return f==File::FirmwarePart?"firmware.part":f==File::ArtPart?"artes.part":f==File::VersionedArt?"artes final":f==File::BackupArt?"artes backup":f==File::FirmwareTag?"firmware.tag":f==File::ArtTag?"artes.tag":f==File::Card?"cartao":"artes.pak";}
inline const char* reason(Issue i){switch(i){case Issue::Mount:return "montar SD";case Issue::Open:return "abrir";case Issue::Size:return "tamanho";case Issue::Read:return "leitura";case Issue::Magic:return "assinatura PKA1";case Issue::Length:return "payload tamanho";case Issue::HeaderCrc:return "CRC cabecalho";case Issue::Catalog:return "catalogo";case Issue::Crc:return "CRC dos dados";case Issue::Sha:return "SHA-256 prefixo";case Issue::HashApi:return "API SHA-256";case Issue::Rename:return "renomear";case Issue::BackupExists:return "backup preservado";default:return "nenhuma";}}
inline bool hexValues(Issue i){return i==Issue::Magic||i==Issue::HeaderCrc||i==Issue::Crc||i==Issue::Sha;}
// RAM-only evidence. A single retry budget covers the whole operation.
struct Recovery {Failure first,last;uint8_t attempts=0;bool recovered=false;
 void note(const Failure& f){if(!first.valid())first=f;last=f;}
};
inline bool ioFailure(const Failure& f){return f.issue==Issue::Read||f.issue==Issue::Mount||(f.issue==Issue::Open&&f.code!=2)||(f.issue==Issue::Size&&f.code!=0&&f.code!=2);}
inline bool integrityFailure(const Failure& f){return (f.issue==Issue::Size&&!f.code)||f.issue==Issue::Magic||f.issue==Issue::Length||f.issue==Issue::HeaderCrc||f.issue==Issue::Catalog||f.issue==Issue::Crc||f.issue==Issue::Sha;}
inline Failure bootArtFailure;inline Recovery bootRecovery;
}
