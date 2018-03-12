using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace gs {

    /// <summary>
    /// This class wraps Win32DragDropHook.dll, which injects drag-and-drop functionality into the running Unity App.
    /// Currently only works on Windows. Properly handles Unicode filenames.
    /// 
    /// Call Initialize() to enable dropping, then register a handler with OnDroppedFilesEvent.
    /// 
    /// Call Shutdown() to disable dropping. You probably should also call this in 
    ///    some Monobehavior's OnApplicationQuit() override, but it's not a disaster if you don't.
    /// 
    /// It's probably not a great idea to enable this in the Unity Editor. 
    /// It does seem to work, but the DLL is never unloaded, and it's not clear
    /// what happens when we repeatedly install the Win32 hooks...
    /// 
    /// </summary>
    public class DragDropHandler
    {
        static bool is_initialized = false;

        /// <summary>
        /// If this is true, we should be receiving Win32 drop events and emitting OnDroppedFilesEvent
        /// </summary>
        public static bool IsInitialized {
            get { return is_initialized; }
        }

        /// <summary>
        /// Call this to turn on receiving drop events. 
        /// If this returns false, something has gone wrong.
        /// </summary>
        public static bool Initialize()
        {
            if (is_initialized)
                return true;

            IntPtr hWnd = GetActiveWindow();
            int retval = InstallDragDropHook(hWnd);

            if (retval > 0) {
                SetActiveCallback(callbackF);
                is_initialized = true;
                return true;
            } else {
                return false;
            }
        }

        /// <summary>
        /// Call this to disable drop events.
        /// </summary>
        public static void Shutdown()
        {
            if (is_initialized) {
                SetActiveCallback(null);
                RemoveDragDropHook();
            }
        }


        public delegate void OnDroppedFilesHandler(List<string> filenames);

        /// <summary>
        /// Register a handler with this event to hear about dropped files
        /// </summary>
        public static OnDroppedFilesHandler OnDroppedFilesEvent;





        /*
         *  Internals
         */

        static void PostDroppedFiles(List<string> filenames)
        {
            if (OnDroppedFilesEvent != null)
                OnDroppedFilesEvent(filenames);
        }
        


        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void DropCallback(int numFiles);

        // we send this callback to the dragdrop DLL
        static DropCallback callbackF = (numFiles) => 
        {
            if (numFiles <= 0)   // happens w/ email drops?
                return;
            List<string> filenames = new List<string>();
            for ( int i = 0; i < numFiles; ++i ) {
                int nChars = GetLastDropFilenameLength(i);
                int nBytes = 2 * (nChars + 1);
                IntPtr bufPtr = Marshal.AllocHGlobal(nBytes);
                int retval = GetLastDropFilename(i, bufPtr, nBytes);
                if (retval > 0)
                    filenames.Add(stringFromChar(bufPtr));
                Marshal.FreeHGlobal(bufPtr);
            }
            PostDroppedFiles(filenames);
        };


        private static string stringFromChar(IntPtr ptr) {
            return Marshal.PtrToStringAuto(ptr);
        }


#if (UNITY_STANDALONE_WIN)
        [DllImport("user32.dll")]
        static extern System.IntPtr GetActiveWindow();

        [DllImport("Win32DragDropHook", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        static extern int InstallDragDropHook(IntPtr hWnd);

        [DllImport("Win32DragDropHook", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        static extern void RemoveDragDropHook();

        [DllImport("Win32DragDropHook", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        static extern void SetActiveCallback([MarshalAs(UnmanagedType.FunctionPtr)] DropCallback callbackF);

        [DllImport("Win32DragDropHook", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        static extern int GetLastDropFilenameLength(int nFile);

        [DllImport("Win32DragDropHook", CharSet = CharSet.Unicode, CallingConvention = CallingConvention.StdCall)]
        static extern int GetLastDropFilename(int nFile, IntPtr buffer, int nBytes);
#endif



    }


}