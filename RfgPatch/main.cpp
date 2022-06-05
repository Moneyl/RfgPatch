#include "common/Typedefs.h"
#include "Common/timing/Timer.h"
#include "Common/filesystem/Path.h"
#include "Common/filesystem/File.h"
#include "common/string/String.h"
#include "common/Defer.h"
#include <RfgTools++/formats/packfiles/Packfile3.h>
#include <iostream>

#pragma warning(disable:4005) //Disable repeat definitions for windows defines
//Target windows 7 with win10 sdk
#include <winsdkver.h>
#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#pragma warning(default:4005)
#include <errhandlingapi.h>
#include <libloaderapi.h>

void UpdateAsmPc(const string& asmPath);
string GetExeFolder();

constexpr u32 RFG_PATCHFILE_SIGNATURE = 1330528590; //Equals ASCII string "NANO"
constexpr u32 RFG_PATCHFILE_VERSION = 1;
const string RFG_PATCH_VERSION = "v1.0.0";

int main(int argc, char* argv[])
{
    /*RfgPatch tool. Takes .RfgPatch file as input and patches the relevant map vpp_pc file. Put this exe in your rfg data folder and drop RfgPatch files onto it to run the patch.*/
    if (argc <= 1)
    {
        printf("RfgPatch %s\n", RFG_PATCH_VERSION.c_str());
        printf("This tool takes a .RfgPatch file as input and applies it to the game files. This tool must be in your RFGR folder or data folder to work.\n");
        printf("To use it either drag and drop a patch onto it, or pass it as an argument on the command line.\n");
        printf("You can drop multiple patch files onto it to apply them all.\n");
        return 0;
    }

    string basePath = GetExeFolder();
    if (std::filesystem::exists(basePath + "rfg.exe"))
        basePath += "data\\"; //Detect if the exe is in the main RFGR folder or the data folder. Works in either

    printf("Base path: %s\n", basePath.c_str());

    //Apply all RfgPatch files passed as inputs
    try
    {
        for (size_t i = 1; i < argc; i++)
        {
            size_t patchCount = argc - 1;
            size_t patchNumber = i;
            string patchPath = argv[i];
            printf("Applying %s...                (%zd/%zd)\n", Path::GetFileName(patchPath).c_str(), patchNumber, patchCount);
            if (!std::filesystem::exists(patchPath))
            {
                printf("  %s doesn't exist. Ignoring...\n", patchPath.c_str());
                continue;
            }
            if (Path::GetExtension(patchPath) != ".RfgPatch")
            {
                printf("  %s isn't an RfgPatch file. Ignoring...\n", patchPath.c_str());
                continue;
            }

            //Load patch file & validate
            BinaryReader patch(patchPath);
            u32 signature = patch.ReadUint32();
            if (signature != RFG_PATCHFILE_SIGNATURE)
            {
                printf("  Patch file '%s' broken. Expected signature %d, found %d\n", patchPath.c_str(), RFG_PATCHFILE_SIGNATURE, signature);
                continue;
            }
            u32 version = patch.ReadUint32();
            if (version != RFG_PATCHFILE_VERSION)
            {
                printf("  Patch file '%s' broken. Expected signature %d, found %d\n", patchPath.c_str(), RFG_PATCHFILE_VERSION, version);
                continue;
            }

            //Load packfile name + make sure it exists. Patch files work by patching existing vpp_pc files. Doesn't work without the vpp_pc
            string packfileName = patch.ReadNullTerminatedString();
            patch.Align(4);
            if (!std::filesystem::exists(basePath + packfileName))
            {
                printf("  Couldn't find %s. It's required by %s. Skipping patch. Make sure RfgPatch.exe is in your RFGR folder.\n", packfileName.c_str(), patchPath.c_str());
                continue;
            }

            //Load zone file sizes
            printf("  Extracting zones from RfgPatch file...\n");
            size_t zoneSize = patch.ReadUint64();
            size_t pZoneSize = patch.ReadUint64();

            //Load zone data
            u8* zoneBytes = new u8[zoneSize];
            u8* pZoneBytes = new u8[pZoneSize];
            patch.ReadToMemory(zoneBytes, zoneSize);
            patch.Align(4);
            patch.ReadToMemory(pZoneBytes, pZoneSize);
            patch.Align(4);
            defer(delete[] zoneBytes);
            defer(delete[] pZoneBytes);

            //Write zone files to current folder
            string mapName = Path::GetFileNameNoExtension(packfileName);
            string zoneFilename = mapName + ".rfgzone_pc";
            string pZoneFilename = "p_" + mapName + ".rfgzone_pc";
            File::WriteToFile(basePath + zoneFilename, { zoneBytes, zoneSize });
            File::WriteToFile(basePath + pZoneFilename, { pZoneBytes, pZoneSize });

            //Backup vpp_pc if it hasn't already been
            printf("  Backing up %s...\n", packfileName.c_str());
            string backupVppPath = basePath + mapName + ".original.vpp_pc";
            if (!std::filesystem::exists(backupVppPath))
                std::filesystem::copy_file(basePath + packfileName, backupVppPath, std::filesystem::copy_options::overwrite_existing);

            //Parse vpp_pc
            printf("  Extracting %s...\n", packfileName.c_str());
            Packfile3 vpp(basePath + packfileName);
            vpp.ReadMetadata();

            //Extract vpp_pc
            string tempFolder = basePath + "PatcherTemp\\";
            string vppFolder = tempFolder + "vpp\\";
            string containerFolder = tempFolder + "containers\\";
            std::filesystem::create_directories(vppFolder);
            std::filesystem::create_directories(tempFolder);
            std::filesystem::create_directories(containerFolder);
            vpp.ExtractSubfiles(vppFolder, false);

            //Extract str2_pc files from vpp_pc
            for (auto entry : std::filesystem::directory_iterator(vppFolder))
            {
                if (!entry.is_regular_file() || entry.path().extension() != ".str2_pc")
                    continue;

                printf("  Extracting %s...\n", Path::GetFileName(entry.path().string()).c_str());
                string str2OutPath = containerFolder + Path::GetFileNameNoExtension(entry.path().filename()) + "\\";
                Packfile3 str2(entry.path().string());
                str2.ExtractSubfiles(str2OutPath, true);
            }

            //Copy zones into vpp_pc and ns_base.str2_pc
            printf("  Copying zones to temp folder...\n");
            std::filesystem::copy_file(basePath + zoneFilename, vppFolder + zoneFilename, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::copy_file(basePath + pZoneFilename, vppFolder + pZoneFilename, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::copy_file(basePath + zoneFilename, containerFolder + "ns_base\\" + zoneFilename, std::filesystem::copy_options::overwrite_existing);

            //Repack str2_pc files
            printf("  Repacking str2_pc files...\n");
            string nsBaseOutPath = vppFolder + "ns_base.str2_pc";
            string minimapOutPath = vppFolder + mapName + "_map.str2_pc";
            std::filesystem::remove(nsBaseOutPath);
            std::filesystem::remove(minimapOutPath);
            Packfile3::Pack(containerFolder + "ns_base\\", nsBaseOutPath, true, true);
            Packfile3::Pack(containerFolder + mapName + "_map\\", minimapOutPath, true, true);
            std::filesystem::remove_all(containerFolder); //Delete containers subfolder. Don't need files after they've been repacked
            
            //Update asm_pc file
            printf("  Updating asm_pc file...\n");
            UpdateAsmPc(vppFolder + mapName + ".asm_pc");

            //Repack main vpp_pc
            printf("  Repacking %s...\n", packfileName.c_str());
            string packfileTempOutPath = tempFolder + packfileName;
            Packfile3::Pack(vppFolder, packfileTempOutPath, false, false);

            //Copy vpp_pc to output path + delete temporary files
            printf("  Cleaning up...\n");
            std::filesystem::copy_file(packfileTempOutPath, basePath + packfileName, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::remove_all(tempFolder);

            //Delete temporary zone files
            std::filesystem::remove(basePath + zoneFilename);
            std::filesystem::remove(basePath + pZoneFilename);
            std::filesystem::remove_all(tempFolder);

            printf("  Done applying %s!\n\n", Path::GetFileName(patchPath).c_str());
        }
    }
    catch (std::exception& ex)
    {
        printf("Exception thrown while applying patches: '%s'\n", ex.what());
    }

    //Require user to press key to close so they see the completion message
    std::cout << "Press enter to close...";
    getchar();

    return 0;
}

void UpdateAsmPc(const string& asmPath)
{
    AsmFile5 asmFile;
    BinaryReader reader(asmPath);
    asmFile.Read(reader, Path::GetFileName(asmPath));

    //Update containers
    for (auto& file : std::filesystem::directory_iterator(Path::GetParentDirectory(asmPath)))
    {
        if (!file.is_regular_file() || file.path().extension() != ".str2_pc")
            continue;

        //Parse container file to get up to date values
        string packfilePath = file.path().string();
        if (!std::filesystem::exists(packfilePath))
            continue;

        Packfile3 packfile(packfilePath);
        packfile.ReadMetadata();

        for (AsmContainer& container : asmFile.Containers)
        {
            struct AsmPrimitiveSizeIndices { size_t HeaderIndex = -1; size_t DataIndex = -1; };
            std::vector<AsmPrimitiveSizeIndices> sizeIndices = {};
            size_t curPrimSizeIndex = 0;
            //Pre-calculate indices of header/data size for each primitive in Container::PrimitiveSizes
            for (AsmPrimitive& prim : container.Primitives)
            {
                AsmPrimitiveSizeIndices& indices = sizeIndices.emplace_back();
                indices.HeaderIndex = curPrimSizeIndex++;
                if (prim.DataSize == 0)
                    indices.DataIndex = -1; //This primitive has no gpu file and so it only has one value in PrimitiveSizes
                else
                    indices.DataIndex = curPrimSizeIndex++;
            }
            const bool virtualContainer = container.CompressedSize == 0 && container.DataOffset == 0;

            if (!virtualContainer && String::EqualIgnoreCase(Path::GetFileNameNoExtension(packfile.Name()), container.Name))
            {
                container.CompressedSize = packfile.Header.CompressedDataSize;

                size_t dataStart = 0;
                dataStart += 2048; //Header size
                dataStart += packfile.Entries.size() * 28; //Each entry is 28 bytes
                dataStart += BinaryWriter::CalcAlign(dataStart, 2048); //Align(2048) after end of entries
                dataStart += packfile.Header.NameBlockSize; //Filenames list
                dataStart += BinaryWriter::CalcAlign(dataStart, 2048); //Align(2048) after end of file names

                printf("  Setting container offset '%s', packfile name: '%s'\n", container.Name.c_str(), packfile.Name().c_str());
                container.DataOffset = dataStart;
            }

            for (size_t entryIndex = 0; entryIndex < packfile.Entries.size(); entryIndex++)
            {
                Packfile3Entry& entry = packfile.Entries[entryIndex];
                for (size_t primitiveIndex = 0; primitiveIndex < container.Primitives.size(); primitiveIndex++)
                {
                    AsmPrimitive& primitive = container.Primitives[primitiveIndex];
                    AsmPrimitiveSizeIndices& indices = sizeIndices[primitiveIndex];
                    if (String::EqualIgnoreCase(primitive.Name, packfile.EntryNames[entryIndex]))
                    {
                        //If DataSize = 0 assume this primitive has no gpu file
                        if (primitive.DataSize == 0)
                        {
                            primitive.HeaderSize = entry.DataSize; //Uncompressed size
                            if (!virtualContainer)
                            {
                                container.PrimitiveSizes[indices.HeaderIndex] = primitive.HeaderSize; //TODO: Just remove PrimitiveSizes from AsmFile and generate it on write
                            }
                        }
                        else //Otherwise assume primitive has cpu and gpu file
                        {
                            primitive.HeaderSize = entry.DataSize; //Cpu file uncompressed size
                            primitive.DataSize = packfile.Entries[entryIndex + 1].DataSize; //Gpu file uncompressed size
                            if (!virtualContainer)
                            {
                                container.PrimitiveSizes[indices.HeaderIndex] = primitive.HeaderSize; //TODO: Just remove PrimitiveSizes from AsmFile and generate it on write
                                container.PrimitiveSizes[indices.DataIndex] = primitive.DataSize; //TODO: Just remove PrimitiveSizes from AsmFile and generate it on write
                            }
                        }
                    }
                }
            }
        }
    }

    asmFile.Write(asmPath);
}

string GetExeFolder()
{
    char path[FILENAME_MAX];
    if (GetModuleFileNameA(NULL, path, FILENAME_MAX) == 0)
        throw std::runtime_error("Failed to get exe folder. Error code: " + std::to_string(GetLastError()));

    return Path::GetParentDirectory(string(path));
}