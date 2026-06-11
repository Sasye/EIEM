#pragma once






#define _CRT_SECURE_NO_WARNINGS
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <windows.h>





struct PmxBone {
  std::string name;         
  std::string englishName;  
  float position[3];        
  int32_t parentIndex;      
  uint16_t flags;
};

struct PmxFile {
  
  float version;
  uint8_t encode;           
  uint8_t boneIndexSize;

  
  std::string modelName;

  
  std::vector<PmxBone> bones;

  bool loaded;
  std::string error;

  PmxFile() : version(0), encode(0), boneIndexSize(2), loaded(false) {}
};




namespace pmx_internal {


static bool ReadBytes(FILE *f, void *buf, size_t n) {
  return fread(buf, 1, n, f) == n;
}


template<typename T>
static bool Read(FILE *f, T *val) {
  return fread(val, sizeof(T), 1, f) == 1;
}


static bool Skip(FILE *f, size_t n) {
  return fseek(f, (long)n, SEEK_CUR) == 0;
}


static bool ReadIndex(FILE *f, int32_t *out, uint8_t indexSize) {
  switch (indexSize) {
    case 1: { uint8_t v;  if (!Read(f, &v))  return false; *out = (v == 0xFF) ? -1 : (int32_t)v; break; }
    case 2: { uint16_t v; if (!Read(f, &v)) return false; *out = (v == 0xFFFF) ? -1 : (int32_t)v; break; }
    case 4: { int32_t v;  if (!Read(f, &v))  return false; *out = v; break; }
    default: return false;
  }
  return true;
}


static bool ReadString(FILE *f, uint8_t encode, std::string *out) {
  uint32_t byteLen = 0;
  if (!Read(f, &byteLen)) return false;
  if (byteLen == 0) { *out = ""; return true; }

  std::vector<uint8_t> buf(byteLen);
  if (!ReadBytes(f, buf.data(), byteLen)) return false;

  if (encode == 0) {
    
    int wcharCount = (int)(byteLen / 2);
    const wchar_t *wstr = (const wchar_t *)buf.data();
    int u8Len = WideCharToMultiByte(CP_UTF8, 0, wstr, wcharCount, NULL, 0, NULL, NULL);
    if (u8Len <= 0) { *out = ""; return true; }
    out->resize(u8Len);
    WideCharToMultiByte(CP_UTF8, 0, wstr, wcharCount, &(*out)[0], u8Len, NULL, NULL);
  } else {
    
    out->assign((const char *)buf.data(), byteLen);
  }
  return true;
}


static bool SkipString(FILE *f) {
  uint32_t byteLen = 0;
  if (!Read(f, &byteLen)) return false;
  return Skip(f, byteLen);
}


static bool SkipVertices(FILE *f, uint8_t addUVNum, uint8_t boneIndexSize) {
  int32_t count = 0;
  if (!Read(f, &count)) return false;

  for (int32_t i = 0; i < count; i++) {
    
    if (!Skip(f, 32)) return false;
    
    if (addUVNum > 0 && !Skip(f, addUVNum * 16)) return false;

    
    uint8_t weightType;
    if (!Read(f, &weightType)) return false;
    switch (weightType) {
      case 0: 
        if (!Skip(f, boneIndexSize)) return false;
        break;
      case 1: 
        if (!Skip(f, boneIndexSize * 2 + 4)) return false;
        break;
      case 2: 
        if (!Skip(f, boneIndexSize * 4 + 16)) return false;
        break;
      case 3: 
        if (!Skip(f, boneIndexSize * 2 + 4 + 36)) return false;
        break;
      case 4: 
        if (!Skip(f, boneIndexSize * 4 + 16)) return false;
        break;
      default:
        return false;
    }
    
    if (!Skip(f, 4)) return false;
  }
  return true;
}


static bool SkipFaces(FILE *f, uint8_t vertexIndexSize) {
  int32_t indexCount = 0;
  if (!Read(f, &indexCount)) return false;
  return Skip(f, (size_t)indexCount * vertexIndexSize);
}


static bool SkipTextures(FILE *f, uint8_t encode) {
  int32_t count = 0;
  if (!Read(f, &count)) return false;
  for (int32_t i = 0; i < count; i++) {
    if (!SkipString(f)) return false;
  }
  return true;
}


static bool SkipMaterials(FILE *f, uint8_t encode, uint8_t textureIndexSize) {
  int32_t count = 0;
  if (!Read(f, &count)) return false;
  for (int32_t i = 0; i < count; i++) {
    
    if (!SkipString(f)) return false;
    if (!SkipString(f)) return false;
    
    if (!Skip(f, 44)) return false;
    
    if (!Skip(f, 1)) return false;
    
    if (!Skip(f, 20)) return false;
    
    if (!Skip(f, textureIndexSize * 2)) return false;
    
    if (!Skip(f, 1)) return false;
    
    uint8_t toonMode;
    if (!Read(f, &toonMode)) return false;
    if (toonMode == 0) {
      
      if (!Skip(f, textureIndexSize)) return false;
    } else {
      
      if (!Skip(f, 1)) return false;
    }
    
    if (!SkipString(f)) return false;
    
    if (!Skip(f, 4)) return false;
  }
  return true;
}

} 




static PmxFile *LoadPmxBones(const char *path) {
  using namespace pmx_internal;

  PmxFile *pmx = new PmxFile();

  FILE *f = fopen(path, "rb");
  if (!f) {
    pmx->error = "Cannot open file";
    return pmx;
  }

  
  
  char magic[4];
  if (!ReadBytes(f, magic, 4) || memcmp(magic, "PMX ", 4) != 0) {
    pmx->error = "Invalid PMX signature";
    fclose(f); return pmx;
  }

  
  if (!Read(f, &pmx->version)) {
    pmx->error = "Cannot read version";
    fclose(f); return pmx;
  }

  
  uint8_t dataSize;
  if (!Read(f, &dataSize)) {
    pmx->error = "Cannot read data size";
    fclose(f); return pmx;
  }

  
  uint8_t addUVNum = 0;
  uint8_t vertexIndexSize = 4;
  uint8_t textureIndexSize = 4;
  uint8_t materialIndexSize = 4;
  uint8_t morphIndexSize = 4;
  uint8_t rigidbodyIndexSize = 4;

  if (!Read(f, &pmx->encode)) { pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &addUVNum))     { pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &vertexIndexSize))   { pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &textureIndexSize))  { pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &materialIndexSize)) { pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &pmx->boneIndexSize)){ pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &morphIndexSize))    { pmx->error = "header"; fclose(f); return pmx; }
  if (!Read(f, &rigidbodyIndexSize)){ pmx->error = "header"; fclose(f); return pmx; }

  
  if (!ReadString(f, pmx->encode, &pmx->modelName)) { pmx->error = "info"; fclose(f); return pmx; }
  
  if (!SkipString(f) || !SkipString(f) || !SkipString(f)) {
    pmx->error = "info strings";
    fclose(f); return pmx;
  }

  
  if (!SkipVertices(f, addUVNum, pmx->boneIndexSize)) {
    pmx->error = "vertices";
    fclose(f); return pmx;
  }

  
  if (!SkipFaces(f, vertexIndexSize)) {
    pmx->error = "faces";
    fclose(f); return pmx;
  }

  
  if (!SkipTextures(f, pmx->encode)) {
    pmx->error = "textures";
    fclose(f); return pmx;
  }

  
  if (!SkipMaterials(f, pmx->encode, textureIndexSize)) {
    pmx->error = "materials";
    fclose(f); return pmx;
  }

  
  int32_t boneCount = 0;
  if (!Read(f, &boneCount) || boneCount < 0 || boneCount > 10000) {
    pmx->error = "bone count";
    fclose(f); return pmx;
  }

  pmx->bones.resize(boneCount);
  for (int32_t i = 0; i < boneCount; i++) {
    PmxBone &bone = pmx->bones[i];

    
    if (!ReadString(f, pmx->encode, &bone.name)) { pmx->error = "bone name"; fclose(f); return pmx; }
    if (!ReadString(f, pmx->encode, &bone.englishName)) { pmx->error = "bone ename"; fclose(f); return pmx; }

    
    if (!ReadBytes(f, bone.position, 12)) { pmx->error = "bone pos"; fclose(f); return pmx; }

    
    if (!ReadIndex(f, &bone.parentIndex, pmx->boneIndexSize)) { pmx->error = "bone parent"; fclose(f); return pmx; }

    
    if (!Skip(f, 4)) { pmx->error = "bone depth"; fclose(f); return pmx; }

    
    if (!Read(f, &bone.flags)) { pmx->error = "bone flags"; fclose(f); return pmx; }

    
    if ((bone.flags & 0x0001) == 0) {
      Skip(f, 12); 
    } else {
      Skip(f, pmx->boneIndexSize); 
    }

    
    if (bone.flags & 0x0100 || bone.flags & 0x0200) {
      Skip(f, pmx->boneIndexSize + 4); 
    }

    
    if (bone.flags & 0x0400) {
      Skip(f, 12); 
    }

    
    if (bone.flags & 0x0800) {
      Skip(f, 24); 
    }

    
    if (bone.flags & 0x2000) {
      Skip(f, 4); 
    }

    
    if (bone.flags & 0x0020) {
      
      Skip(f, pmx->boneIndexSize + 4 + 4);
      
      int32_t linkCount = 0;
      if (!Read(f, &linkCount)) { pmx->error = "ik links"; fclose(f); return pmx; }
      for (int32_t j = 0; j < linkCount; j++) {
        Skip(f, pmx->boneIndexSize); 
        uint8_t hasLimit;
        if (!Read(f, &hasLimit)) { pmx->error = "ik limit"; fclose(f); return pmx; }
        if (hasLimit) {
          Skip(f, 24); 
        }
      }
    }
  }

  fclose(f);
  pmx->loaded = true;
  return pmx;
}

static void FreePmx(PmxFile *pmx) {
  if (pmx) delete pmx;
}




static void DumpPmxBones(const PmxFile *pmx, FILE *out) {
  fprintf(out, "=== PMX Bone Dump ===\n");
  fprintf(out, "Model: %s\n", pmx->modelName.c_str());
  fprintf(out, "Version: %.1f, Encode: %s\n",
    pmx->version, pmx->encode == 0 ? "UTF-16" : "UTF-8");
  fprintf(out, "Bones: %zu\n\n", pmx->bones.size());

  for (size_t i = 0; i < pmx->bones.size(); i++) {
    const PmxBone &b = pmx->bones[i];
    fprintf(out, "[%3zu] %-30s parent=%3d  pos=(%.4f, %.4f, %.4f)  flags=0x%04x\n",
      i, b.name.c_str(), b.parentIndex,
      b.position[0], b.position[1], b.position[2],
      b.flags);
  }
}
