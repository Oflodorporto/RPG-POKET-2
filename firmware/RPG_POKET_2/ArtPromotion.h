#pragma once
#include <errno.h>
enum class ArtPresence {Absent,Present,Uncertain};
template<class FS> ArtPresence artPresence(FS& fs,const char* path){errno=0;bool present=fs.exists(path);int code=errno;if(present)return code?ArtPresence::Uncertain:ArtPresence::Present;return code==ENOENT?ArtPresence::Absent:ArtPresence::Uncertain;}
// Names only, no deletion. Call with the SPI mutex and all handles closed.
// A prior backup is never replaced. Boot can read backup if final is absent/bad.
enum class PromotionIssue {None,Lookup,BackupExists,Rename};
struct ArtPromotion {bool movedOld=false,movedNew=false,backupPresent=false;PromotionIssue issue=PromotionIssue::None;int error=0;};
template<class FS> bool promoteArt(FS& fs,const char* part,const char* final,const char* backup,ArtPromotion& state){
 auto fail=[&](PromotionIssue issue){state.issue=issue;state.error=errno;return false;};
 auto backupState=artPresence(fs,backup);if(backupState==ArtPresence::Uncertain)return fail(PromotionIssue::Lookup);state.backupPresent=backupState==ArtPresence::Present;
 auto finalState=artPresence(fs,final);if(finalState==ArtPresence::Uncertain)return fail(PromotionIssue::Lookup);
 if(finalState==ArtPresence::Present){
  if(state.backupPresent){errno=0;return fail(PromotionIssue::BackupExists);}
  errno=0;if(!fs.rename(final,backup))return fail(PromotionIssue::Rename);
  state.movedOld=state.backupPresent=true;
 }
 errno=0;if(!fs.rename(part,final)){
  fail(PromotionIssue::Rename); // Preserve the failed promotion, not rollback errno.
  if(state.movedOld)fs.rename(backup,final);
  return false;
 }
 state.movedNew=true;return true;
}
template<class FS> bool restoreArt(FS& fs,const char* part,const char* final,const char* backup,const ArtPromotion& state){
 if(!state.movedNew)return true;
 if(artPresence(fs,part)!=ArtPresence::Absent)return false;
 errno=0;if(!fs.rename(final,part))return false;
 if(state.movedOld){errno=0;return fs.rename(backup,final);}
 return true; // Existing backup remains available to boot, never overwritten.
}
