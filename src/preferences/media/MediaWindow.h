// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        MediaWindow.h
//  Author:      Sikosis, Jérôme Duval
//  Description: Media Preferences
//  Created :    June 25, 2003
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
#ifndef __MEDIAWINDOWS_H__
#define __MEDIAWINDOWS_H__


#include <Box.h>
#include <ListView.h>
#include <MediaAddOn.h>
#include <ParameterWeb.h>
#include <StringView.h>
#include <Window.h>

#include <ObjectList.h>

#include "MediaAlert.h"
#include "MediaIcons.h"
#include "MediaListItem.h"
#include "MediaViews.h"


#define SETTINGS_FILE "MediaPrefs Settings"


class BSeparatorView;
// struct dormant_node_info;


class MediaWindow : public BWindow
{
public:
								MediaWindow(BRect frame);
   								~MediaWindow();
    		status_t			InitCheck();

	// methods to be called by MediaListItems...
			void				SelectNode(dormant_node_info* node);
			void				SelectAudioSettings(const char* title);
			void				SelectVideoSettings(const char* title);
			void				SelectAudioMixer(const char* title);

    virtual	bool 				QuitRequested();
    virtual	void				MessageReceived(BMessage* message);
    virtual	void				FrameResized(float width, float height);


private:

	typedef BObjectList<dormant_node_info> NodeList;


			status_t			InitMedia(bool first);
			void				_FindNodes();
			void				_FindNodes(media_type type, uint64 kind,
									NodeList& into);	
			void				_AddNodeItems(NodeList &from,
									MediaListItem::media_type type);
			void				_EmptyNodeLists();

			NodeListItem*		_FindNodeListItem(dormant_node_info* info);
			void				InitWindow();

	static	status_t			RestartMediaServices(void* data);
	static	bool				UpdateProgress(int stage, const char * message,
									void * cookie);

			void				_ClearParamView();
			void				_MakeParamView();
			void				_MakeEmptyParamView();

	struct SmartNode {
								SmartNode(const BMessenger& notifyHandler);
								~SmartNode();
			void				SetTo(dormant_node_info* node);
			void				SetTo(const media_node& node);
			bool				IsSet();
								operator media_node();

	private:
			void				_FreeNode();
			media_node*			fNode;
			BMessenger			fMessenger;
	};
	
			BBox*				fBox;
			BListView*			fListView;
			BSeparatorView*		fTitleView;
			BView*				fContentView;
			SettingsView*		fAudioView;
			SettingsView*		fVideoView;
    			    
			SmartNode			fCurrentNode;
			BParameterWeb*		fParamWeb;
			

			NodeList			fAudioInputs;
			NodeList			fAudioOutputs;
			NodeList			fVideoInputs;
			NodeList			fVideoOutputs;
	
			MediaAlert*			fAlert;
			status_t			fInitCheck;
};

#endif
