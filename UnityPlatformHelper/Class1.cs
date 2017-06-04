using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace UnityPlatformHelper
{
    public class OpenFileTest
    {
        public void DoImportSTLDialog()
        {
            OpenFileDialog dlg = new OpenFileDialog();
            DialogResult result = dlg.ShowDialog();
        }
    }
}
