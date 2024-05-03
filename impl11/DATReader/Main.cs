﻿using JeremyAnsel.Xwa.Dat;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

/*
 
 DAT Image Formats:

 - Format 7: 8-bit indexed colors and 1-bit alpha, RLE compressed. 
   Use: concourse or in-flight.

 - Format 23: 8-bit indexed colors and 8-bit alpha, RLE compressed.
   Use: concourse.

 - Format 24: 8-bit indexed colors and 8-bit alpha (uncompressed?) Use: in-flight.

 - Format 25: 32-bit ARGB (uncompressed?) Use: in-flight.

 */

namespace DATReader
{
    public static class Main
    {
        static Dictionary<string, DatFile> m_GenericDatFiles = new Dictionary<string, DatFile>(StringComparer.OrdinalIgnoreCase);
        static HashSet<string> m_GenericDatNames = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "GenericDamage",
            "RubbleEffects",
        };

        // Cached DatFile var. This must be loaded before we start inspecting the file
        static DatFile m_DATFile = null;
        // Cached image var. This must be loaded before accessing raw image data.
        // This var is loaded in GetDATImageMetadata below
        static DatImage m_DATImage = null;
        // Enable verbosity
        static bool m_Verbose = false;

        [DllExport(CallingConvention.Cdecl)]
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

        [DllExport(CallingConvention.Cdecl)]
        public static void SetDATVerbosity(bool Verbose)
        {
            m_Verbose = Verbose;
        }

        [DllExport(CallingConvention.Cdecl)]
        public static bool LoadDATFile([MarshalAs(UnmanagedType.LPStr)] string sDatFileName)
        {
            // First, let's check if this DAT file is already loaded:
            if (m_DATFile != null && m_DATFile.FileName.ToLower().Equals(sDatFileName.ToLower()))
            {
                if (m_Verbose) Trace.WriteLine("[DBG] [C#] DAT File " + sDatFileName + " already loaded");
                return true;
            }

            if (m_Verbose) Trace.WriteLine("[DBG] [C#] Loading File: " + sDatFileName);
            try
            {
                m_DATImage = null; // Release any previous instances
                m_DATFile = null; // Release any previous instances
                //m_DATFile = DatFile.FromFile(sDatFileName);

                string name = Path.GetFileNameWithoutExtension(sDatFileName);

                if (!m_GenericDatFiles.TryGetValue(name, out m_DATFile))
                {
                    if (m_GenericDatNames.Contains(name))
                    {
                        m_DATFile = DatFile.FromFile(sDatFileName, true);
                        //m_DATFile.ConvertToFormat25();
                        m_GenericDatFiles.Add(name, m_DATFile);
                    }
                    else
                    {
                        m_DATFile = DatFile.FromFile(sDatFileName, false);
                    }
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

        [DllExport(CallingConvention.Cdecl)]
        public static unsafe bool GetDATImageMetadata(int GroupId, int ImageId, short* Width, short* Height, byte* Format)
        {
            if (m_DATFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Load a DAT file first");
                *Width = 0; *Height = 0; *Format = 0;
                return false;
            }

            DatImage image = m_DATFile.GetImageById((short)GroupId, (short)ImageId);

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

        [DllExport(CallingConvention.Cdecl)]
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

            if (!m_DATFile.HasImagesData)
            {
                m_DATFile = DatFile.FromFile(m_DATFile.FileName, true);
                m_DATImage = m_DATFile.GetImageById(m_DATImage.GroupId, m_DATImage.ImageId);
            }

            short W = m_DATImage.Width;
            short H = m_DATImage.Height;
            byte[] data;

            if (RawData_size == W * H || m_DATImage.Format == DatImageFormat.Format25)
            {
                data = m_DATImage.GetRawData();
            }
            else
            {
                data = m_DATImage.GetImageData();
            }

            if (m_Verbose)
            {
                Trace.WriteLine("[DBG] [C#] RawData, W*H*4 = " + (W * H * 4) + ", len: " + data.Length + ", Format: " + m_DATImage.Format);
            }

            Marshal.Copy(data, 0, new IntPtr(RawData_out), data.Length);
            return true;
        }

        // Legacy function. Deprecated. To be used only with old DAT files.
        [DllExport(CallingConvention.Cdecl)]
        public static unsafe bool ReadFlippedDATImageData(byte* RawData_out, int RawData_size)
        {
            if (m_DATImage == null || m_DATFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Must cache an image first");
                return false;
            }

            //m_DATImage = DatFile.GetImageDataById(m_DATFile.FileName, m_DATImage.GroupId, m_DATImage.ImageId);

            if (!m_DATFile.HasImagesData)
            {
                m_DATFile = DatFile.FromFile(m_DATFile.FileName, true);
                m_DATImage = m_DATFile.GetImageById(m_DATImage.GroupId, m_DATImage.ImageId);
            }

            //m_DATImage.ConvertToFormat25(); // Looks like there's no need to do any conversion
            short W = m_DATImage.Width;
            short H = m_DATImage.Height;

            if (RawData_size == W * H)
            {
                Marshal.Copy(m_DATImage.GetRawData(), 0, new IntPtr(RawData_out), m_DATImage.GetRawData().Length);
                return true;
            }

            byte[] data = m_DATImage.Format == DatImageFormat.Format25 ? m_DATImage.GetRawData() : m_DATImage.GetImageData();

            int len = data.Length;
            if (m_Verbose)
                Trace.WriteLine("[DBG] [C#] RawData, W*H*4 = " + (W * H * 4) + ", len: " + len + ", Format: " + m_DATImage.Format);

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

        [DllExport(CallingConvention.Cdecl)]
        public static int GetDATGroupImageCount(int GroupId)
        {
            if (m_DATFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Load a DAT file first");
                return 0;
            }

            DatGroup group = m_DATFile.GetGroupById((short)GroupId);

            if (group != null)
            {
                return group.Images.Count;
            }

            return 0;
        }

        [DllExport(CallingConvention.Cdecl)]
        public static unsafe bool GetDATGroupImageList(int GroupId, short* ImageIds_out, int ImageIds_size)
        {
            if (m_DATFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Load a DAT file first");
                return false;
            }

            DatGroup group = m_DATFile.GetGroupById((short)GroupId);

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
    }
}
