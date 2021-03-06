﻿using JocysCom.ClassLibrary.Controls.IssuesControl;
using System.Linq;
using x360ce.Engine;

namespace x360ce.App.Issues
{
	class HidGuardianDriverIssue : IssueItem
	{
		public HidGuardianDriverIssue() : base()
		{
			Name = "HID Guardian Driver";
			FixName = "Install";
		}

		public override void CheckTask()
		{
			var required = SettingsManager.UserGames.Items.Any(x => x.EmulationType == (int)EmulationType.Virtual);
			if (!required)
			{
				SetSeverity(IssueSeverity.None);
				return;
			}
			var hid = DInput.VirtualDriverInstaller.GetHidGuardianDriverInfo();
			if (hid.DriverVersion == 0)
			{
				SetSeverity(IssueSeverity.Critical, 0, "You need to install HID Guardian Driver to hide DirectInput controllers.");
				return;
			}
			SetSeverity(IssueSeverity.None);
		}
		public override void FixTask()
		{
			Program.RunElevated(AdminCommand.InstallHidGuardian);
			ViGEm.HidGuardianHelper.InsertCurrentProcessToWhiteList();
		}

	}
}
