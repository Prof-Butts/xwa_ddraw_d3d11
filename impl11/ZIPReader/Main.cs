using System;
using System.Collections;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;

/*

 To configure projects like these for the first time, you need to run 

    DllExport.bat -action Configure

 Then set the right namespace (ZIPReader in this case). The project needs to be 
 reloaded and recompiled.

 */

namespace ZIPReader
{
    public static class Main
    {
        static string m_sZIPFileName = null;
        static ZipArchive m_ZIPFile = null;
        // Enable verbosity
        static bool m_Verbose = false;

        [DllExport(CallingConvention.Cdecl)]
        public static void SetZIPVerbosity(bool Verbose)
        {
            m_Verbose = Verbose;
        }

        static readonly string[] ImageExtensions = { ".png", ".jpg", ".gif" };

        static string FindUnzippedImage(int GroupId, int ImageId)
        {
            if (m_ZIPFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Load a ZIP file first");
                return null;
            }

            string sPath = GroupId.ToString(CultureInfo.InvariantCulture) + "/" + ImageId.ToString(CultureInfo.InvariantCulture);
            //Trace.WriteLineIf(m_Verbose, "[DBG] [C#] Searching for image " + GroupId + "-" + ImageId + " in [" + sPath + "]");
            foreach (var Extension in ImageExtensions)
            {
                string sFullPath = sPath + Extension;

                if (m_ZIPFile.GetEntry(sFullPath) != null)
                {
                    //Trace.WriteLineIf(m_Verbose, "[DBG] [C#] Found image: [" + sFullPath + "] for " + GroupId + "-" + ImageId);
                    return sFullPath;
                }
            }

            return null;
        }

        [DllExport(CallingConvention.Cdecl)]
        public static bool LoadZIPFile([MarshalAs(UnmanagedType.LPStr)] string sZIPFileName)
        {
            bool result = false;

            // First, let's check if this ZIP file is already loaded:
            if (string.Equals(m_sZIPFileName, sZIPFileName, StringComparison.OrdinalIgnoreCase))
            {
                Trace.WriteLineIf(m_Verbose, "[DBG] [C#] ZIP File " + sZIPFileName + " already loaded");
                return true;
            }

            m_ZIPFile?.Dispose();
            m_ZIPFile = null;
            m_sZIPFileName = null;

            try
            {
                Trace.WriteLineIf(m_Verbose, "[DBG] [C#] Loading File: " + sZIPFileName);
                m_ZIPFile = ZipFile.OpenRead(sZIPFileName);
                if (m_ZIPFile == null)
                {
                    Trace.WriteLineIf(m_Verbose, "[DBG] [C#] Failed when loading: " + sZIPFileName);
                    return false;
                }

                // "Cache" the name of the ZIP file and the temp path for future reference
                m_sZIPFileName = sZIPFileName;
                result = true;
            }
            catch (Exception e)
            {
                Trace.WriteLine("[DBG] [C#] Exception " + e + " when opening " + sZIPFileName);
                result = false;
            }

            return result;
        }

        [DllExport(CallingConvention.Cdecl)]
        public static unsafe bool GetZIPImageMetadata(int GroupId, int ImageId, short* Width_out, short* Height_out, byte** ImageData_out, int* ImageDataSize_out)
        {
            string sImagePath = FindUnzippedImage(GroupId, ImageId);
            if (sImagePath == null)
                return false;

            ZipArchiveEntry zipEntry = m_ZIPFile.GetEntry(sImagePath);

            if (ImageData_out != null && ImageDataSize_out != null)
            {
                using (Stream zipImage = zipEntry.Open())
                {
                    int length = (int)zipEntry.Length;
                    byte[] data = new byte[length];
                    zipImage.Read(data, 0, length);

                    IntPtr bufferPtr = Marshal.AllocHGlobal(length);
                    Marshal.Copy(data, 0, bufferPtr, length);

                    *ImageData_out = (byte*)bufferPtr;
                    *ImageDataSize_out = length;
                }
            }

            if (Height_out != null || Width_out != null)
            {
                using (Stream zipImage = zipEntry.Open())
                {
                    using (var image = Image.FromStream(zipImage, false, false))
                    {
                        if (Height_out != null) *Height_out = (short)image.Height;
                        if (Width_out != null) *Width_out = (short)image.Width;
                    }
                }
            }

            return true;
        }

        [DllExport(CallingConvention.Cdecl)]
        public static int GetZIPGroupImageCount(int GroupId)
        {
            if (m_ZIPFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Load a ZIP file first");
                return 0;
            }

            int result = 0;

            try
            {
                string groupPath = GroupId.ToString(CultureInfo.InvariantCulture) + "/";
                result = m_ZIPFile.Entries.Count(t => t.FullName.StartsWith(groupPath, StringComparison.OrdinalIgnoreCase));
            }
            catch (Exception e)
            {
                Trace.WriteLine("[DBG] [C#] Exception in GetZIPGroupImageCount: " + e.ToString());
                result = 0;
            }

            return result;
        }

        static int GetImageIdFromImageName(string sImageName)
        {
            int result = -1;
            string sRoot = Path.GetFileNameWithoutExtension(sImageName);
            if (int.TryParse(sRoot, out result))
                return result;
            else
                return -1;
        }

        [DllExport(CallingConvention.Cdecl)]
        public static unsafe bool GetZIPGroupImageList(int GroupId, short* ImageIds_out, int ImageIds_size)
        {
            if (m_ZIPFile == null)
            {
                Trace.WriteLine("[DBG] [C#] Load a ZIP file first");
                return false;
            }

            string groupPath = GroupId.ToString(CultureInfo.InvariantCulture) + "/";
            var sImageNames = m_ZIPFile.Entries
                .Where(t => t.FullName.StartsWith(groupPath, StringComparison.OrdinalIgnoreCase))
                .Select(t => t.Name);

            ArrayList ImageIds = new ArrayList();
            foreach (string sImageName in sImageNames)
            {
                ImageIds.Add(GetImageIdFromImageName(sImageName));
            }
            ImageIds.Sort();

            int Ofs = 0;
            foreach (int ImageId in ImageIds)
            {
                ImageIds_out[Ofs] = (short)ImageId;
                // Advance the output index, but prevent an overflow
                if (Ofs < ImageIds_size) Ofs++;
            }

            return true;
        }

        /*
         * Removes all the temp directories created when unzipping files
         */
        [DllExport(CallingConvention.Cdecl)]
        public static void DeleteAllTempZIPDirectories()
        {
            Trace.WriteLine("[DBG] [C#] Deleting all temporary directories...");

            m_ZIPFile?.Dispose();
            m_ZIPFile = null;
            m_sZIPFileName = null;
        }
    }
}
