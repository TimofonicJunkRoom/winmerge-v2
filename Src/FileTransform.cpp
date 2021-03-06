/////////////////////////////////////////////////////////////////////////////
//    WinMerge:  an interactive diff/merge utility
//    Copyright (C) 1997-2000  Thingamahoochie Software
//    Author: Dean Grimm
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
/////////////////////////////////////////////////////////////////////////////
/**
 *  @file FileTransform.cpp
 *
 *  @brief Implementation of file transformations
 */ 

#include "FileTransform.h"
#include <vector>
#include <Poco/Exception.h>
#include "Plugins.h"
#include "paths.h"
#include "multiformatText.h"
#include "UniMarkdownFile.h"
#include "Environment.h"
#include "TFile.h"

using Poco::Exception;

int g_bUnpackerMode = PLUGIN_MANUAL;
int g_bPredifferMode = PLUGIN_MANUAL;





////////////////////////////////////////////////////////////////////////////////
// transformations : packing unpacking

// known handler
bool FileTransform_Packing(String & filepath, PackingInfo handler)
{
	// no handler : return true
	if (handler.pluginName.empty())
		return true;

	storageForPlugins bufferData;
	bufferData.SetDataFileAnsi(filepath);

	// control value
	bool bHandled = false;

	PluginInfo * plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"FILE_PACK_UNPACK", handler.pluginName);
	if (plugin == NULL)
		plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"FILE_FOLDER_PACK_UNPACK", handler.pluginName);
	if (plugin == NULL)
		plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"BUFFER_PACK_UNPACK", handler.pluginName);
	LPDISPATCH piScript = plugin->m_lpDispatch;
	if (handler.bWithFile)
	{
		// use a temporary dest name
		String srcFileName = bufferData.GetDataFileAnsi(); // <-Call order is important
		String dstFileName = bufferData.GetDestFileName(); // <-Call order is important
		bHandled = InvokePackFile(srcFileName,
			dstFileName,
			bufferData.GetNChanged(),
			piScript, handler.subcode);
		if (bHandled)
			bufferData.ValidateNewFile();
	}
	else
	{
		bHandled = InvokePackBuffer(*bufferData.GetDataBufferAnsi(),
			bufferData.GetNChanged(),
			piScript, handler.subcode);
		if (bHandled)
			bufferData.ValidateNewBuffer();
	}

	// if this packer does not work, that is an error
	if (bHandled == false)
		return false;

	// if the buffer changed, write it before leaving
	bool bSuccess = true;
	if (bufferData.GetNChangedValid() > 0)
	{
		bSuccess = bufferData.SaveAsFile(filepath);
	}

	return bSuccess;
}

// known handler
bool FileTransform_Unpacking(String & filepath, const PackingInfo * handler, int * handlerSubcode)
{
	// no handler : return true
	if (!handler || handler->pluginName.empty())
		return true;

	storageForPlugins bufferData;
	bufferData.SetDataFileAnsi(filepath);

	// temporary subcode 
	int subcode;

	// control value
	bool bHandled = false;

	PluginInfo * plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"FILE_PACK_UNPACK", handler->pluginName);
	if (plugin == NULL)
		plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"FILE_FOLDER_PACK_UNPACK", handler->pluginName);
	if (plugin == NULL)
		plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"BUFFER_PACK_UNPACK", handler->pluginName);
	if (plugin == NULL)
		return false;

	LPDISPATCH piScript = plugin->m_lpDispatch;
	if (handler->bWithFile)
	{
		// use a temporary dest name
		String srcFileName = bufferData.GetDataFileAnsi(); // <-Call order is important
		String dstFileName = bufferData.GetDestFileName(); // <-Call order is important
		bHandled = InvokeUnpackFile(srcFileName,
			dstFileName,
			bufferData.GetNChanged(),
			piScript, subcode);
		if (bHandled)
			bufferData.ValidateNewFile();
	}
	else
	{
		bHandled = InvokeUnpackBuffer(*bufferData.GetDataBufferAnsi(),
			bufferData.GetNChanged(),
			piScript, subcode);
		if (bHandled)
			bufferData.ValidateNewBuffer();
	}

	// if this unpacker does not work, that is an error
	if (bHandled == false)
		return false;

	// valid the subcode
	*handlerSubcode = subcode;

	// if the buffer changed, write it before leaving
	bool bSuccess = true;
	if (bufferData.GetNChangedValid() > 0)
	{
		bSuccess = bufferData.SaveAsFile(filepath);
	}

	return bSuccess;
}


// scan plugins for the first handler
bool FileTransform_Unpacking(String & filepath, const String& filteredText, PackingInfo * handler, int * handlerSubcode)
{
	// PLUGIN_BUILTIN_XML : read source file through custom UniFile
	if (handler->bToBeScanned == PLUGIN_BUILTIN_XML)
	{
		handler->pufile = new UniMarkdownFile;
		handler->textType = _T("xml");
		handler->disallowMixedEOL = true;
		handler->pluginName.erase(); // Make FileTransform_Packing() a NOP
		// Leave bToBeScanned alone so above lines will continue to execute on
		// subsequent calls to this function
		return true;
	}

	storageForPlugins bufferData;
	bufferData.SetDataFileAnsi(filepath);

	// control value
	bool bHandled = false;

	PluginInfo * plugin = CAllThreadsScripts::GetActiveSet()->GetAutomaticPluginByFilter(L"FILE_PACK_UNPACK", filteredText);
	if (!plugin)
		plugin = CAllThreadsScripts::GetActiveSet()->GetAutomaticPluginByFilter(L"FILE_FOLDER_PACK_UNPACK", filteredText);
	if (plugin)
	{
		handler->pluginName = plugin->m_name;
		handler->bWithFile = true;
		// use a temporary dest name
		String srcFileName = bufferData.GetDataFileAnsi(); // <-Call order is important
		String dstFileName = bufferData.GetDestFileName(); // <-Call order is important
		bHandled = InvokeUnpackFile(srcFileName,
			dstFileName,
			bufferData.GetNChanged(),
			plugin->m_lpDispatch, handler->subcode);
		if (bHandled)
			bufferData.ValidateNewFile();
	}

	// We can not assume that the file is text, so use a safearray and not a BSTR
	// TODO : delete this event ? 	Is anyone going to use this ?

	if (!bHandled)
	{
		plugin = CAllThreadsScripts::GetActiveSet()->GetAutomaticPluginByFilter(L"BUFFER_PACK_UNPACK", filteredText);
		if (plugin)
		{
			handler->pluginName = plugin->m_name;
			handler->bWithFile = false;
			bHandled = InvokeUnpackBuffer(*bufferData.GetDataBufferAnsi(),
				bufferData.GetNChanged(),
				plugin->m_lpDispatch, handler->subcode);
			if (bHandled)
				bufferData.ValidateNewBuffer();
		}
	}

	if (bHandled == false)
	{
		// we didn't find any unpacker, just hope it is normal Ansi/Unicode
		handler->pluginName = _T("");
		handler->subcode = 0;
		bHandled = true;
	}

	// the handler is now defined
	handler->bToBeScanned = false;

	// assign the sucode
	*handlerSubcode = handler->subcode;

	// if the buffer changed, write it before leaving
	bool bSuccess = true;
	if (bufferData.GetNChangedValid() > 0)
	{
		bSuccess = bufferData.SaveAsFile(filepath);
	}

	return bSuccess;
}

bool FileTransform_Unpacking(PackingInfo *handler, String& filepath, const String& filteredText)
{
	if (handler->bToBeScanned)
		return FileTransform_Unpacking(filepath, filteredText, handler, &handler->subcode);
	else
		return FileTransform_Unpacking(filepath, handler, &handler->subcode);
}

////////////////////////////////////////////////////////////////////////////////
// transformation prediffing
    
// known handler
bool FileTransform_Prediffing(String & filepath, PrediffingInfo handler, bool bMayOverwrite)
{
	// no handler : return true
	if (handler.pluginName.empty())
		return true;

	storageForPlugins bufferData;
	// detect Ansi or Unicode file
	bufferData.SetDataFileUnknown(filepath, bMayOverwrite);
	// TODO : set the codepage
	// bufferData.SetCodepage();

	// control value
	bool bHandled = false;

	PluginInfo * plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"FILE_PREDIFF", handler.pluginName);
	if (!plugin)
	{
		plugin = CAllThreadsScripts::GetActiveSet()->GetPluginByName(L"BUFFER_PREDIFF", handler.pluginName);
		if (!plugin)
			return false;
	}
	LPDISPATCH piScript = plugin->m_lpDispatch;
	if (handler.bWithFile)
	{
		// use a temporary dest name
		String srcFileName = bufferData.GetDataFileAnsi(); // <-Call order is important
		String dstFileName = bufferData.GetDestFileName(); // <-Call order is important
		bHandled = InvokePrediffFile(srcFileName,
			dstFileName,
			bufferData.GetNChanged(),
			piScript);
		if (bHandled)
			bufferData.ValidateNewFile();
	}
	else
	{
		// probably it is for VB/VBscript so use a BSTR as argument
		bHandled = InvokePrediffBuffer(*bufferData.GetDataBufferUnicode(),
			bufferData.GetNChanged(),
			piScript);
		if (bHandled)
			bufferData.ValidateNewBuffer();
	}

	// if this unpacker does not work, that is an error
	if (bHandled == false)
		return false;

	// if the buffer changed, write it before leaving
	bool bSuccess = true;
	if (bufferData.GetNChangedValid() > 0)
	{
		// bufferData changes filepath here to temp filepath
		bSuccess = bufferData.SaveAsFile(filepath);
	}

	return bSuccess;
}


// scan plugins for the first handler
bool FileTransform_Prediffing(String & filepath, const String& filteredText, PrediffingInfo * handler, bool bMayOverwrite)
{
	storageForPlugins bufferData;
	// detect Ansi or Unicode file
	bufferData.SetDataFileUnknown(filepath, bMayOverwrite);
	// TODO : set the codepage
	// bufferData.SetCodepage();

	// control value
	bool bHandled = false;

	PluginInfo * plugin = CAllThreadsScripts::GetActiveSet()->GetAutomaticPluginByFilter(L"FILE_PREDIFF", filteredText);
	if (plugin)
	{
		handler->pluginName = plugin->m_name;
		handler->bWithFile = true;
		// use a temporary dest name
		String srcFileName = bufferData.GetDataFileAnsi(); // <-Call order is important
		String dstFileName = bufferData.GetDestFileName(); // <-Call order is important
		bHandled = InvokePrediffFile(srcFileName,
			dstFileName,
			bufferData.GetNChanged(),
			plugin->m_lpDispatch);
		if (bHandled)
			bufferData.ValidateNewFile();
	}

	if (!bHandled)
	{
		plugin = CAllThreadsScripts::GetActiveSet()->GetAutomaticPluginByFilter(L"BUFFER_PREDIFF", filteredText);
		if (plugin)
		{
			handler->pluginName = plugin->m_name;
			handler->bWithFile = false;
			// probably it is for VB/VBscript so use a BSTR as argument
			bHandled = InvokePrediffBuffer(*bufferData.GetDataBufferUnicode(),
				bufferData.GetNChanged(),
				plugin->m_lpDispatch);
			if (bHandled)
				bufferData.ValidateNewBuffer();
		}
	}

	if (bHandled == false)
	{
		// we didn't find any prediffer, that is OK anyway
		handler->pluginName = _T("");
		bHandled = true;
	}

	// the handler is now defined
	handler->bToBeScanned = false;

	// if the buffer changed, write it before leaving
	bool bSuccess = true;
	if (bufferData.GetNChangedValid() > 0)
	{
		bSuccess = bufferData.SaveAsFile(filepath);
	}

	return bSuccess;
}

bool FileTransform_Prediffing(PrediffingInfo * handler, String & filepath, const String& filteredText, bool bMayOverwrite)
{
	if (handler->bToBeScanned)
		return FileTransform_Prediffing(filepath, filteredText, handler, bMayOverwrite);
	else
		return FileTransform_Prediffing(filepath, *handler, bMayOverwrite);
}


////////////////////////////////////////////////////////////////////////////////

bool FileTransform_AnyCodepageToUTF8(int codepage, String & filepath, bool bMayOverwrite)
{
	String tempDir = env_GetTempPath();
	if (tempDir.empty())
		return false;
	String tempFilepath = env_GetTempFileName(tempDir, _T("_W3"));
	if (tempFilepath.empty())
		return false;
	// TODO : is it better with the BOM or without (just change the last argument)
	int nFileChanged = 0;
	bool bSuccess = AnyCodepageToUTF8(codepage, filepath, tempFilepath, nFileChanged, false); 
	if (bSuccess && nFileChanged)
	{
		// we do not overwrite so we delete the old file
		if (bMayOverwrite)
		{
			try
			{
				TFile(filepath).remove();
			}
			catch (Exception& e)
			{
				LogErrorStringUTF8(e.displayText());
			}
		}
		// and change the filepath if everything works
		filepath = tempFilepath;
	}
	else
	{
		try
		{
			TFile(tempFilepath).remove();
		}
		catch (Exception& e)
		{
			LogErrorStringUTF8(e.displayText());
		}
	}

	return bSuccess;
}


////////////////////////////////////////////////////////////////////////////////
// transformation : TextTransform_Interactive (editor scripts)

void GetFreeFunctionsInScripts(std::vector<String>& sNamesArray, const wchar_t *TransformationEvent)
{
	// get an array with the available scripts
	PluginArray * piScriptArray = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(TransformationEvent);

	// fill in these structures
	int nFnc = 0;	
	int iScript;
	for (iScript = 0 ; iScript < piScriptArray->size() ; iScript++)
	{
		const PluginInfoPtr & plugin = piScriptArray->at(iScript);
		if (plugin->m_disabled)
			continue;
		LPDISPATCH piScript = plugin->m_lpDispatch;
		std::vector<String> scriptNamesArray;
		std::vector<int> scriptIdsArray;
		int nScriptFnc = GetMethodsFromScript(piScript, scriptNamesArray, scriptIdsArray);
		sNamesArray.resize(nFnc+nScriptFnc);

		int iFnc;
		for (iFnc = 0 ; iFnc < nScriptFnc ; iFnc++)
			sNamesArray[nFnc+iFnc] = scriptNamesArray[iFnc];

		nFnc += nScriptFnc;
	}
}

bool TextTransform_Interactive(String & text, const wchar_t *TransformationEvent, int iFncChosen)
{
	if (iFncChosen < 0)
		return false;

	// get an array with the available scripts
	PluginArray * piScriptArray = 
		CAllThreadsScripts::GetActiveSet()->GetAvailableScripts(TransformationEvent);

	int iScript;
	for (iScript = 0 ; iScript < piScriptArray->size() ; iScript++)
	{
		if (iFncChosen < piScriptArray->at(iScript)->m_nFreeFunctions)
			// we have found the script file
			break;
		iFncChosen -= piScriptArray->at(iScript)->m_nFreeFunctions;
	}

	if (iScript >= piScriptArray->size())
		return false;

	// iFncChosen is the index of the function in the script file
	// we must convert it to the function ID
	int fncID = GetMethodIDInScript(piScriptArray->at(iScript)->m_lpDispatch, iFncChosen);

	// execute the transform operation
	int nChanged = 0;
	InvokeTransformText(text, nChanged, piScriptArray->at(iScript)->m_lpDispatch, fncID);

	return (nChanged != 0);
}

////////////////////////////////////////////////////////////////////////////////
