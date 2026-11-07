using JeremyAnsel.Xwa.Dat;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DATReader;

/*
 
 DAT Image Formats:

 - Format 7: 8-bit indexed colors and 1-bit alpha, RLE compressed. 
   Use: concourse or in-flight.

 - Format 23: 8-bit indexed colors and 8-bit alpha, RLE compressed.
   Use: concourse.

 - Format 24: 8-bit indexed colors and 8-bit alpha (uncompressed?) Use: in-flight.

 - Format 25: 32-bit ARGB (uncompressed?) Use: in-flight.

 */

/*
 * A DAT file whose images are read one at a time. It holds the metadata of all the images
 * and where the data of each image is in the file, so reading an image doesn't load the
 * rest of the file. XWA is a 32-bit process and some DAT files take more than 100 MB.
 */
sealed class IndexedDatFile
{
    // The groups and images, without their image data
    public DatFile Metadata { get; private set; }
    public string FileName => Metadata.FileName;

    // (GroupId, ImageId) --> where the image data is in the file
    readonly Dictionary<int, (long Offset, int Length)> m_Index = new Dictionary<int, (long, int)>();

    static int Key(short groupId, short imageId) => (groupId << 16) | (ushort)imageId;

    public static IndexedDatFile FromFile(string fileName)
    {
        var datFile = new IndexedDatFile();
        datFile.Metadata = DatFile.FromFile(fileName, false);
        datFile.BuildIndex(fileName);
        return datFile;
    }

    // Walks the image headers to find where the data of each image is. This follows the same
    // layout as JeremyAnsel.Xwa.Dat's DatFile.FromStream.
    void BuildIndex(string fileName)
    {
        using var stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
        using var reader = new BinaryReader(stream);
        if (reader.ReadInt64() != 6197554890949403479L)
            throw new InvalidDataException("Not a DAT file: " + fileName);

        short version = reader.ReadInt16();
        short groupsCount = reader.ReadInt16();
        reader.ReadInt16();
        reader.ReadInt32();
        if (version != 0)
            stream.Position += 12;
        stream.Position += 4;

        int imagesCount = 0;
        for (int i = 0; i < groupsCount; i++)
        {
            reader.ReadInt16(); // GroupId
            imagesCount += reader.ReadInt16();
            reader.ReadInt32();
            if (version != 0)
                stream.Position += 12;
            reader.ReadInt32();
        }

        for (int i = 0; i < imagesCount; i++)
        {
            stream.Position += 10; // Format, Width, Height and two unknown fields
            short groupId = reader.ReadInt16();
            short imageId = reader.ReadInt16();
            // The size includes the rest of the header (44 bytes) and the image data
            int size = reader.ReadInt32();
            if (size < 44)
                throw new InvalidDataException("Invalid image size in " + fileName);
            long dataOffset = stream.Position + 44;
            // Keep the first image when an id is repeated, like DatFile.GetImageById
            m_Index.TryAdd(Key(groupId, imageId), (dataOffset, size - 44));
            stream.Position = dataOffset + size - 44;
        }
    }

    // Reads the data of one image. metadata must come from this file's Metadata.
    public DatImage ReadImage(DatImage metadata)
    {
        if (!m_Index.TryGetValue(Key(metadata.GroupId, metadata.ImageId), out var entry))
            return null;

        byte[] rawData = new byte[entry.Length];
        using (var stream = new FileStream(FileName, FileMode.Open, FileAccess.Read, FileShare.Read, 1))
        {
            stream.Position = entry.Offset;
            stream.ReadExactly(rawData);
        }
        // The metadata already has the final format (for instance, Format25 with 2 colors is BC3)
        return DatImage.FromMemory(metadata.GroupId, metadata.ImageId, metadata.Format,
            metadata.Width, metadata.Height, metadata.ColorsCount, rawData);
    }
}

public static class Main
{
    // The generic DAT files are used all the time, so their index stays loaded
    static Dictionary<string, IndexedDatFile> m_GenericDatFiles = new Dictionary<string, IndexedDatFile>(StringComparer.OrdinalIgnoreCase);
    static HashSet<string> m_GenericDatNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "GenericDamage",
            "RubbleEffects",
        };

    // Cached DAT file. This must be loaded before we start inspecting the file
    static IndexedDatFile m_DATFile = null;
    // Metadata of the cached image (without its image data). This must be loaded before
    // accessing raw image data. This var is loaded in GetDATImageMetadata below
    static DatImage m_DATImage = null;
    // Enable verbosity
    static bool m_Verbose = false;

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetDATReaderVersion))]
    public static uint GetDATReaderVersion()
    {
        Version version = typeof(Main).Assembly.GetName().Version;
        //Trace.WriteLine("[DBG] [C#] DATReader version: " + version.ToString());
        //Trace.WriteLine("[DBG] [C#] Major: " + version.Major);
        //Trace.WriteLine("[DBG] [C#] Minor: " + version.Minor);
        //Trace.WriteLine("[DBG] [C#] Build: " + version.Build);
        int V = (version.Major << 16) | (version.Minor << 8) | version.Build;
        return (uint)V;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(SetDATVerbosity))]
    public static void SetDATVerbosity(bool Verbose)
    {
        m_Verbose = Verbose;
    }

    // Returns the memory that's no longer used, mostly the buffers of the images read so far.
    // XWA is a 32-bit process and its own allocations don't trigger a collection, so this
    // garbage can keep XWA from finding large blocks of free memory unless it's released.
    static void ReleaseUnusedMemory()
    {
        long start = Stopwatch.GetTimestamp();
        GCSettings.LargeObjectHeapCompactionMode = GCLargeObjectHeapCompactionMode.CompactOnce;
        GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced, true, true);
        m_LastCollectionMs = (float)Stopwatch.GetElapsedTime(start).TotalMilliseconds;
    }

    // How long the collection done by the last LoadDATFile or UnloadDATFile call took, in
    // milliseconds. It's 0 if that call didn't collect. ddraw logs it.
    static float m_LastCollectionMs = 0;

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetLastCollectionTimeMs))]
    public static float GetLastCollectionTimeMs()
    {
        return m_LastCollectionMs;
    }

    // Releases the cached DAT file and image. The index of the generic DAT files stays loaded.
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(UnloadDATFile))]
    public static void UnloadDATFile()
    {
        m_LastCollectionMs = 0;
        if (m_Verbose) Trace.WriteLine("[DBG] [C#] Unloading the cached DAT file");
        m_DATImage = null;
        m_DATFile = null;
        ReleaseUnusedMemory();
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(LoadDATFile))]
    public static bool LoadDATFile(nint sDatFileNamePtr)
    {
        string sDatFileName = Marshal.PtrToStringAnsi(sDatFileNamePtr);
        m_LastCollectionMs = 0;

        // First, let's check if this DAT file is already loaded:
        if (m_DATFile != null && m_DATFile.FileName.ToLower().Equals(sDatFileName.ToLower()))
        {
            if (m_Verbose) Trace.WriteLine("[DBG] [C#] DAT File " + sDatFileName + " already loaded");
            return true;
        }

        if (m_Verbose) Trace.WriteLine("[DBG] [C#] Loading File: " + sDatFileName);
        try
        {
            bool releasePrevious = m_DATFile != null;
            m_DATImage = null; // Release any previous instances
            m_DATFile = null; // Release any previous instances
                              //m_DATFile = DatFile.FromFile(sDatFileName);
            // Free the previous DAT file before loading the next one
            if (releasePrevious)
                ReleaseUnusedMemory();

            string name = Path.GetFileNameWithoutExtension(sDatFileName);

            if (!m_GenericDatFiles.TryGetValue(name, out m_DATFile))
            {
                // Only the metadata and the index are loaded, the images are read when needed
                m_DATFile = IndexedDatFile.FromFile(sDatFileName);
                if (m_GenericDatNames.Contains(name))
                    m_GenericDatFiles.Add(name, m_DATFile);
            }

            if (m_DATFile == null)
            {
                if (m_Verbose) Trace.WriteLine("[DBG] [C#] Failed when loading: " + sDatFileName);
                return false;
            }
        }
        catch (Exception e)
        {
            Trace.WriteLine("[DBG] [C#] Exception " + e + " when opening " + sDatFileName);
            m_DATFile = null;
        }
        return true;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetDATImageMetadata))]
    public static unsafe bool GetDATImageMetadata(int GroupId, int ImageId, short* Width, short* Height, byte* Format)
    {
        if (m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Load a DAT file first");
            *Width = 0; *Height = 0; *Format = 0;
            return false;
        }

        DatImage image = m_DATFile.Metadata.GetImageById((short)GroupId, (short)ImageId);

        if (image != null)
        {
            // Release the previous cached image
            m_DATImage = null;
            // Cache the current image
            m_DATImage = image;
            // Populate the output values
            *Width = image.Width;
            *Height = image.Height;
            *Format = (byte)image.Format;
            if (m_Verbose) Trace.WriteLine("[DBG] [C#] Found " + GroupId + "-" + ImageId + ", " +
                "MetaData: (" + image.Width + ", " + image.Height + "), Type: " + image.Format);
            return true;
        }

        return false;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(ReadDATImageData))]
    public static unsafe bool ReadDATImageData(byte* RawData_out, int RawData_size)
    {
        if (m_DATImage == null || m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Must cache an image first");
            return false;
        }

        if (RawData_out == null)
        {
            Trace.WriteLine("[DBG] [C#] ReadDATImageData: output buffer should not be NULL");
            return false;
        }

        // Read only this image from the file. Its data isn't kept after this call.
        DatImage image = ReadCachedImage();
        if (image == null)
            return false;

        short W = image.Width;
        short H = image.Height;
        byte[] data;

        if (RawData_size == W * H || image.Format == DatImageFormat.Format25)
        {
            data = image.GetRawData();
        }
        else
        {
            data = image.GetImageData();
        }

        if (image.Format == DatImageFormat.FormatBc5)
        {
            // Format is BGRA with B = 0 and A = 255 (for opaque colors).
            // For this format, we assume it's a normal map, so we reconstruct
            // the blue channel here:
            int pixels = W * H;
            int Ofs = 0;
            for (int i = 0; i < pixels; i++)
            {
                float B = data[Ofs + 0];
                float G = data[Ofs + 1] / 255.0f;
                float R = data[Ofs + 2] / 255.0f;
                B = (float)Math.Sqrt(1.0f - G * G - R * R);
                data[Ofs + 0] = (byte)(B * 255.0f);
                Ofs += 4;
            }
        }

        if (m_Verbose)
        {
            Trace.WriteLine("[DBG] [C#] RawData, W*H*4 = " + (W * H * 4) + ", len: " + data.Length + ", Format: " + image.Format);
        }

        Marshal.Copy(data, 0, new IntPtr(RawData_out), data.Length);
        return true;
    }

    // Reads the data of the image selected by GetDATImageMetadata
    static DatImage ReadCachedImage()
    {
        try
        {
            DatImage image = m_DATFile.ReadImage(m_DATImage);
            if (image == null)
                Trace.WriteLine("[DBG] [C#] Image " + m_DATImage.GroupId + "-" + m_DATImage.ImageId + " not found in " + m_DATFile.FileName);
            return image;
        }
        catch (Exception e)
        {
            Trace.WriteLine("[DBG] [C#] Exception " + e + " when reading an image from " + m_DATFile.FileName);
            return null;
        }
    }

    // Legacy function. Deprecated. To be used only with old DAT files.
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(ReadFlippedDATImageData))]
    public static unsafe bool ReadFlippedDATImageData(byte* RawData_out, int RawData_size)
    {
        if (m_DATImage == null || m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Must cache an image first");
            return false;
        }

        // Read only this image from the file. Its data isn't kept after this call.
        DatImage image = ReadCachedImage();
        if (image == null)
            return false;

        //image.ConvertToFormat25(); // Looks like there's no need to do any conversion
        short W = image.Width;
        short H = image.Height;

        if (RawData_size == W * H)
        {
            Marshal.Copy(image.GetRawData(), 0, new IntPtr(RawData_out), image.GetRawData().Length);
            return true;
        }

        byte[] data = image.Format == DatImageFormat.Format25 ? image.GetRawData() : image.GetImageData();

        int len = data.Length;
        if (m_Verbose)
            Trace.WriteLine("[DBG] [C#] RawData, W*H*4 = " + (W * H * 4) + ", len: " + len + ", Format: " + image.Format);

        if (RawData_out == null)
        {
            Trace.WriteLine("[DBG] [C#] ReadDATImageData: output buffer should not be NULL");
            return false;
        }

        try
        {
            int min_len = RawData_size;
            if (data.Length < min_len) min_len = data.Length;
            // For some reason, the images are still upside down when used as SRVs
            // So, let's flip them here. RowOfs and RowStride are used to flip the
            // image by reading it "backwards".
            UInt32 OfsOut = 0, OfsIn = 0, RowStride = (UInt32)W * 4, RowOfs = (UInt32)(H - 1) * RowStride;
            for (int y = 0; y < H; y++)
            {
                OfsIn = RowOfs; // Flip the image
                for (int x = 0; x < W; x++)
                {
                    RawData_out[OfsOut + 2] = data[OfsIn + 0]; // B
                    RawData_out[OfsOut + 1] = data[OfsIn + 1]; // G
                    RawData_out[OfsOut + 0] = data[OfsIn + 2]; // R
                    RawData_out[OfsOut + 3] = data[OfsIn + 3]; // A
                    OfsIn += 4;
                    OfsOut += 4;
                }
                // Flip the image and prevent underflows:
                RowOfs -= RowStride;
                if (RowOfs < 0) RowOfs = 0;
            }
        }
        catch (Exception e)
        {
            Trace.WriteLine("[DBG] [C#] Exception: " + e + ", caught in ReadDATImageData");
            return false;
        }

        return true;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetDATGroupImageCount))]
    public static int GetDATGroupImageCount(int GroupId)
    {
        if (m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Load a DAT file first");
            return 0;
        }

        DatGroup group = m_DATFile.Metadata.GetGroupById((short)GroupId);

        if (group != null)
        {
            return group.Images.Count;
        }

        return 0;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetDATGroupImageList))]
    public static unsafe bool GetDATGroupImageList(int GroupId, short* ImageIds_out, int ImageIds_size)
    {
        if (m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Load a DAT file first");
            return false;
        }

        DatGroup group = m_DATFile.Metadata.GetGroupById((short)GroupId);

        if (group != null)
        {
            int Ofs = 0;
            foreach (var image in group.Images)
            {
                ImageIds_out[Ofs] = image.ImageId;
                if (m_Verbose) Trace.WriteLine("[DBG] [C#] Stored ImageId: " + image.ImageId);
                // Advance the output index, but prevent an overflow
                if (Ofs < ImageIds_size) Ofs++;
            }
            return true;
        }

        return false;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetDATGroupCount))]
    public static unsafe int GetDATGroupCount()
    {
        if (m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Load a DAT file first");
            return -1;
        }

        return m_DATFile.Metadata.Groups.Count;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)], EntryPoint = nameof(GetDATGroupList))]
    public static unsafe bool GetDATGroupList(short* GroupIds_out)
    {
        if (m_DATFile == null)
        {
            Trace.WriteLine("[DBG] [C#] Load a DAT file first");
            return false;
        }

        int i = 0;
        foreach (var group in m_DATFile.Metadata.Groups)
        {
            GroupIds_out[i++] = group.GroupId;
        }
        return true;
    }
}
