//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------
USEFORM("UnitMain.cpp", FormMain);
USEFORM("UnitTranspose.cpp", FormTranspose);
USEFORM("UnitReplace.cpp", FormReplace);
USEFORM("UnitSectionName.cpp", FormName);
USEFORM("UnitSectionList.cpp", FormSectionList);
USEFORM("UnitSubSong.cpp", FormSubSong);
USEFORM("UnitOutputMonitor.cpp", FormOutputMonitor);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	try
	{
		Application->Initialize();
		Application->Title = "SNES GSS";
		Application->CreateForm(__classid(TFormMain), &FormMain);
		Application->CreateForm(__classid(TFormOutputMonitor), &FormOutputMonitor);
		Application->CreateForm(__classid(TFormTranspose), &FormTranspose);
		Application->CreateForm(__classid(TFormReplace), &FormReplace);
		Application->CreateForm(__classid(TFormName), &FormName);
		Application->CreateForm(__classid(TFormSubSong), &FormSubSong);
		Application->CreateForm(__classid(TFormSectionList), &FormSectionList);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
