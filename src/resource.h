/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#define IDI_ICON                    1
#define IDI_TRAY                    2
#define IDI_TRAY_RECORDING          3
#define IDI_TASKBAR_RECORDING       4
#define IDI_PROGRAMGUIDE            5
#define IDI_SEARCH                  6

#define IDI_TASK_FULLSCREEN         31000
#define IDI_TASK_DISABLEVIEWER      31001
#define IDI_TASK_PROGRAMGUIDE       31002

#define IDI_FAVORITES               31010
#define IDI_FAVORITES_ADD           31011
#define IDI_FAVORITES_FOLDER        31012

#define IDI_PANSCAN_AUTO            31020
#define IDI_PANSCAN_16x9            31021
#define IDI_PANSCAN_LETTERBOX       31022
#define IDI_PANSCAN_WINDOWBOX       31023
#define IDI_PANSCAN_PILLARBOX       31024
#define IDI_PANSCAN_4x3             31025
#define IDI_PANSCAN_32x9            31026
#define IDI_PANSCAN_16x9_LEFT       31027
#define IDI_PANSCAN_16x9_RIGHT      31028
#define IDI_PANSCAN_TOUCHOUTSIDE    31029
#define IDI_PANSCAN_OPTIONS         31030

#define IDC_GRAB1                   10
#define IDC_GRAB2                   11

#define IDB_LOGO                    12
#define IDB_LOGO32                  13
#define IDB_CAPTURE16               14
#define IDB_CAPTURE32               15
#define IDB_TITLEBAR12              16
#define IDB_TITLEBAR24              17
#define IDB_STATUSBAR_FAVORITES16   18
#define IDB_STATUSBAR_FAVORITES32   19
#define IDB_LOGOFRAME16             20
#define IDB_LOGOFRAME32             21
#define IDB_SIDEBAR16               22
#define IDB_SIDEBARZOOM16           23
#define IDB_SIDEBAR32               24
#define IDB_SIDEBARZOOM32           25
#define IDB_OPTIONS16               26
#define IDB_OPTIONS32               27
#define IDB_CHEVRON10               28
#define IDB_CHEVRON20               29
#define IDB_PROGRAMGUIDEICONS       30
#define IDB_PASSTHROUGH16           31
#define IDB_PASSTHROUGH32           32
#define IDB_HOME                    33
#define IDB_PANELTABICONS16         34
#define IDB_PANELTABICONS32         35

#define IDM_MENU                    30000
#define IDM_RECORD                  30001
#define IDM_CAPTURE                 30002
#define IDM_BUFFERING               30003
#define IDM_ERROR                   30004
#define IDM_TIME                    30005
#define IDM_PROGRAMINFOSTATUS       30006
#define IDM_PROGRAMGUIDE            30007
#define IDM_CHANNELSCAN             30008
#define IDM_TRAY                    30009
#define IDM_CAPTUREPREVIEW          30010
#define IDM_PLUGIN                  30011
#define IDM_CHANNELPANEL            30012
#define IDM_INFORMATIONPANEL        30013
#define IDM_SIDEBAR                 30014
#define IDM_COLORSCHEME             30015
#define IDM_CAPTIONPANEL            30016
#define IDM_PROGRAMSEARCH           30017
#define IDM_PROGRAMGUIDETOOLBAR     30018
#define IDM_FEATUREDEVENTS          30019
#define IDM_PROGRAMLISTPANEL        30020
#define IDM_EVENTSEARCHKEYWORD      30021

#define IDD_ABOUT                   40
#define IDD_INITIALSETTINGS         41
#define IDD_OPTIONS                 42
#define IDD_OPTIONS_GENERAL         43
#define IDD_OPTIONS_VIEW            44
#define IDD_OPTIONS_OSD             45
#define IDD_OPTIONS_STATUS          46
#define IDD_OPTIONS_SIDEBAR         47
#define IDD_OPTIONS_MENU            48
#define IDD_OPTIONS_PANEL           49
#define IDD_OPTIONS_COLORSCHEME     50
#define IDD_OPTIONS_OPERATION       51
#define IDD_OPTIONS_ACCELERATOR     52
#define IDD_OPTIONS_CONTROLLER      53
#define IDD_OPTIONS_DRIVER          54
#define IDD_OPTIONS_VIDEO           55
#define IDD_OPTIONS_AUDIO           56
#define IDD_OPTIONS_PLAYBACK        57
#define IDD_OPTIONS_RECORD          58
#define IDD_OPTIONS_CAPTURE         59
#define IDD_OPTIONS_CHANNELSCAN     60
#define IDD_OPTIONS_EPG             61
#define IDD_OPTIONS_PROGRAMGUIDE    62
#define IDD_OPTIONS_PLUGIN          63
#define IDD_OPTIONS_TSPROCESSOR     64
#define IDD_OPTIONS_LOG             65
#define IDD_RECORDOPTION            70
#define IDD_SAVECOLORSCHEME         71
#define IDD_CHANNELSCAN             72
#define IDD_CHANNELPROPERTIES       73
#define IDD_CHANNELSCANSETTINGS     74
#define IDD_PROGRAMGUIDETOOL        75
#define IDD_EVENTSEARCH             76
#define IDD_PROGRAMSEARCH           77
#define IDD_EPGCHANNELSETTINGS      78
#define IDD_ERROR                   79
#define IDD_STREAMPROPERTIES        80
#define IDD_STREAMINFO              81
#define IDD_PIDINFO                 82
#define IDD_TSPROCESSORERROR        83
#define IDD_ZOOMOPTIONS             84
#define IDD_PANANDSCANOPTIONS       85
#define IDD_ORGANIZEFAVORITES       86
#define IDD_FAVORITEPROPERTIES      87
#define IDD_PROGRAMGUIDEFAVORITES   88
#define IDD_PROGRAMGUIDETOOLBAR     89
#define IDD_FEATUREDEVENTS          90
#define IDD_FEATUREDEVENTSSEARCH    91
#define IDD_TSPROCESSOR_TUNERMAP    92
#define IDD_PROPERTYPAGEFRAME       93
#define IDD_SURROUNDOPTIONS         94
#define IDD_CHANNELINPUTOPTIONS     95
#define IDD_DIRECTWRITEOPTIONS      96

#define CM_COMMAND_FIRST                        100
#define CM_ZOOM_FIRST                           100
#define CM_ZOOM_20                              (CM_ZOOM_FIRST + 0)
#define CM_ZOOM_25                              (CM_ZOOM_FIRST + 1)
#define CM_ZOOM_33                              (CM_ZOOM_FIRST + 2)
#define CM_ZOOM_50                              (CM_ZOOM_FIRST + 3)
#define CM_ZOOM_66                              (CM_ZOOM_FIRST + 4)
#define CM_ZOOM_75                              (CM_ZOOM_FIRST + 5)
#define CM_ZOOM_100                             (CM_ZOOM_FIRST + 6)
#define CM_ZOOM_150                             (CM_ZOOM_FIRST + 7)
#define CM_ZOOM_200                             (CM_ZOOM_FIRST + 8)
#define CM_ZOOM_250                             (CM_ZOOM_FIRST + 9)
#define CM_ZOOM_300                             (CM_ZOOM_FIRST + 10)
#define CM_ZOOM_LAST                            (CM_ZOOM_FIRST + 10)
#define CM_ZOOMOPTIONS                          119
#define CM_ASPECTRATIO_TOGGLE                   120
#define CM_ASPECTRATIO_FIRST                    121
#define CM_ASPECTRATIO_DEFAULT                  (CM_ASPECTRATIO_FIRST + 0)
#define CM_ASPECTRATIO_16x9                     (CM_ASPECTRATIO_FIRST + 1)
#define CM_ASPECTRATIO_LETTERBOX                (CM_ASPECTRATIO_FIRST + 2)
#define CM_ASPECTRATIO_WINDOWBOX                (CM_ASPECTRATIO_FIRST + 3)
#define CM_ASPECTRATIO_PILLARBOX                (CM_ASPECTRATIO_FIRST + 4)
#define CM_ASPECTRATIO_4x3                      (CM_ASPECTRATIO_FIRST + 5)
#define CM_ASPECTRATIO_LAST                     (CM_ASPECTRATIO_FIRST + 5)
#define CM_ASPECTRATIO_3D_FIRST                 (CM_ASPECTRATIO_LAST + 1)
#define CM_ASPECTRATIO_32x9                     (CM_ASPECTRATIO_3D_FIRST + 0)
#define CM_ASPECTRATIO_16x9_LEFT                (CM_ASPECTRATIO_3D_FIRST + 1)
#define CM_ASPECTRATIO_16x9_RIGHT               (CM_ASPECTRATIO_3D_FIRST + 2)
#define CM_ASPECTRATIO_3D_LAST                  (CM_ASPECTRATIO_3D_FIRST + 2)
#define CM_PANANDSCANOPTIONS                    135
#define CM_FRAMECUT                             136
#define CM_FULLSCREEN                           137
#define CM_ALWAYSONTOP                          138
#define CM_VOLUME_UP                            139
#define CM_VOLUME_DOWN                          140
#define CM_VOLUME_MUTE                          141
#define CM_DUALMONO_MAIN                        142
#define CM_DUALMONO_SUB                         143
#define CM_DUALMONO_BOTH                        144
#define CM_SWITCHAUDIO                          145
#define CM_SPDIF_DISABLED                       146
#define CM_SPDIF_PASSTHROUGH                    147
#define CM_SPDIF_AUTO                           148
#define CM_SPDIF_TOGGLE                         149
#define CM_RECORD                               150
#define CM_RECORD_START                         151
#define CM_RECORD_STOP                          152
#define CM_RECORD_PAUSE                         153
#define CM_RECORDOPTION                         154
#define CM_RECORDEVENT                          155
#define CM_OPTIONS_RECORD                       156
#define CM_EXITONRECORDINGSTOP                  157
#define CM_TIMESHIFTRECORDING                   158
#define CM_ENABLETIMESHIFTRECORDING             159
#define CM_STATUSBARRECORD                      160
#define CM_DISABLEVIEWER                        161
#define CM_COPYIMAGE                            162
#define CM_SAVEIMAGE                            163
#define CM_CAPTURE                              164
#define CM_CAPTUREPREVIEW                       165
#define CM_CAPTUREOPTIONS                       166
#define CM_OPENCAPTUREFOLDER                    167
#define CM_CAPTURESIZE_ORIGINAL                 170
#define CM_CAPTURESIZE_VIEW                     171
#define CM_CAPTURESIZE_75                       172
#define CM_CAPTURESIZE_66                       173
#define CM_CAPTURESIZE_50                       174
#define CM_CAPTURESIZE_33                       175
#define CM_CAPTURESIZE_25                       176
#define CM_CAPTURESIZE_1920X1080                177
#define CM_CAPTURESIZE_1440X810                 178
#define CM_CAPTURESIZE_1280X720                 179
#define CM_CAPTURESIZE_1024X576                 180
#define CM_CAPTURESIZE_960x540                  181
#define CM_CAPTURESIZE_800X450                  182
#define CM_CAPTURESIZE_640X360                  183
#define CM_CAPTURESIZE_320X180                  184
#define CM_CAPTURESIZE_1440X1080                185
#define CM_CAPTURESIZE_1280X960                 186
#define CM_CAPTURESIZE_1024X768                 187
#define CM_CAPTURESIZE_800X600                  188
#define CM_CAPTURESIZE_720X540                  189
#define CM_CAPTURESIZE_640X480                  190
#define CM_CAPTURESIZE_320X240                  191
#define CM_CAPTURESIZE_FIRST                    CM_CAPTURESIZE_ORIGINAL
#define CM_CAPTURESIZE_LAST                     CM_CAPTURESIZE_320X240
#define CM_RESET                                200
#define CM_RESETVIEWER                          201
#define CM_REBUILDVIEWER                        202
#define CM_PANEL                                203
#define CM_PROGRAMGUIDE                         204
#define CM_TITLEBAR                             205
#define CM_STATUSBAR                            206
#define CM_SIDEBAR                              207
#define CM_WINDOWFRAME_NORMAL                   208
#define CM_WINDOWFRAME_CUSTOM                   209
#define CM_WINDOWFRAME_NONE                     210
#define CM_CUSTOMTITLEBAR                       211
#define CM_SPLITTITLEBAR                        212
#define CM_OPTIONS                              214
#define CM_VIDEODECODERPROPERTY                 215
#define CM_VIDEORENDERERPROPERTY                216
#define CM_AUDIOFILTERPROPERTY                  217
#define CM_AUDIORENDERERPROPERTY                218
#define CM_DEMULTIPLEXERPROPERTY                219
#define CM_STREAMINFO                           220
#define CM_CLOSE                                221
#define CM_EXIT                                 222
#define CM_SHOW                                 223
#define CM_HOMEDISPLAY                          224
#define CM_CHANNELDISPLAY                       225
#define CM_CHANNEL_UP                           226
#define CM_CHANNEL_DOWN                         227
#define CM_CHANNEL_BACKWARD                     228
#define CM_CHANNEL_FORWARD                      229
#define CM_CHANNEL_PREVIOUS                     230
#define CM_UPDATECHANNELLIST                    231
#define CM_MENU                                 232
#define CM_ACTIVATE                             233
#define CM_MINIMIZE                             234
#define CM_MAXIMIZE                             235
#define CM_1SEGMODE                             236
#define CM_CLOSETUNER                           237
#define CM_ENABLEBUFFERING                      240
#define CM_RESETBUFFER                          241
#define CM_RESETERRORCOUNT                      242
#define CM_SHOWRECORDREMAINTIME                 245
#define CM_SHOWTOTTIME                          250
#define CM_INTERPOLATETOTTIME                   251
#define CM_ADJUSTTOTTIME                        252
#define CM_PROGRAMINFOSTATUS_POPUPINFO          253
#define CM_PROGRAMINFOSTATUS_SHOWPROGRESS       254
#define CM_POPUPTITLEBAR                        255
#define CM_POPUPSTATUSBAR                       256
#define CM_CAPTURESTATUSBAR                     269
#define CM_ZOOMMENU                             270
#define CM_ASPECTRATIOMENU                      271
#define CM_CHANNELMENU                          272
#define CM_SERVICEMENU                          273
#define CM_TUNINGSPACEMENU                      274
#define CM_FAVORITESMENU                        275
#define CM_RECENTCHANNELMENU                    276
#define CM_VOLUMEMENU                           277
#define CM_AUDIOMENU                            278
#define CM_VIDEOMENU                            279
#define CM_RESETMENU                            280
#define CM_BARMENU                              281
#define CM_PLUGINMENU                           282
#define CM_FILTERPROPERTYMENU                   283

#define CM_SIDEBAR_PLACE_FIRST                  290
#define CM_SIDEBAR_PLACE_LEFT                   (CM_SIDEBAR_PLACE_FIRST + 0)
#define CM_SIDEBAR_PLACE_RIGHT                  (CM_SIDEBAR_PLACE_FIRST + 1)
#define CM_SIDEBAR_PLACE_TOP                    (CM_SIDEBAR_PLACE_FIRST + 2)
#define CM_SIDEBAR_PLACE_BOTTOM                 (CM_SIDEBAR_PLACE_FIRST + 3)
#define CM_SIDEBAR_PLACE_LAST                   (CM_SIDEBAR_PLACE_FIRST + 3)
#define CM_SIDEBAROPTIONS                       294

#define CM_AUDIOGAIN_FIRST                      800
#define CM_AUDIOGAIN_NONE                       (CM_AUDIOGAIN_FIRST + 0)
#define CM_AUDIOGAIN_125                        (CM_AUDIOGAIN_FIRST + 1)
#define CM_AUDIOGAIN_150                        (CM_AUDIOGAIN_FIRST + 2)
#define CM_AUDIOGAIN_200                        (CM_AUDIOGAIN_FIRST + 3)
#define CM_AUDIOGAIN_LAST                       (CM_AUDIOGAIN_FIRST + 3)
#define CM_SURROUNDAUDIOGAIN_FIRST              810
#define CM_SURROUNDAUDIOGAIN_NONE               (CM_SURROUNDAUDIOGAIN_FIRST + 0)
#define CM_SURROUNDAUDIOGAIN_125                (CM_SURROUNDAUDIOGAIN_FIRST + 1)
#define CM_SURROUNDAUDIOGAIN_150                (CM_SURROUNDAUDIOGAIN_FIRST + 2)
#define CM_SURROUNDAUDIOGAIN_200                (CM_SURROUNDAUDIOGAIN_FIRST + 3)
#define CM_SURROUNDAUDIOGAIN_LAST               (CM_SURROUNDAUDIOGAIN_FIRST + 3)

#define CM_AUDIODELAY_MINUS                     820
#define CM_AUDIODELAY_PLUS                      821
#define CM_AUDIODELAY_RESET                     822

#define CM_INFORMATIONPANEL_ITEM_FIRST          830
#define CM_INFORMATIONPANEL_ITEM_VIDEO          (CM_INFORMATIONPANEL_ITEM_FIRST + 0)
#define CM_INFORMATIONPANEL_ITEM_VIDEODECODER   (CM_INFORMATIONPANEL_ITEM_FIRST + 1)
#define CM_INFORMATIONPANEL_ITEM_VIDEORENDERER  (CM_INFORMATIONPANEL_ITEM_FIRST + 2)
#define CM_INFORMATIONPANEL_ITEM_AUDIODEVICE    (CM_INFORMATIONPANEL_ITEM_FIRST + 3)
#define CM_INFORMATIONPANEL_ITEM_SIGNALLEVEL    (CM_INFORMATIONPANEL_ITEM_FIRST + 4)
#define CM_INFORMATIONPANEL_ITEM_MEDIABITRATE   (CM_INFORMATIONPANEL_ITEM_FIRST + 5)
#define CM_INFORMATIONPANEL_ITEM_ERROR          (CM_INFORMATIONPANEL_ITEM_FIRST + 6)
#define CM_INFORMATIONPANEL_ITEM_RECORD         (CM_INFORMATIONPANEL_ITEM_FIRST + 7)
#define CM_INFORMATIONPANEL_ITEM_SERVICE        (CM_INFORMATIONPANEL_ITEM_FIRST + 8)
#define CM_INFORMATIONPANEL_ITEM_PROGRAMINFO    (CM_INFORMATIONPANEL_ITEM_FIRST + 9)

#define CM_CHANNELPANEL_UPDATE                  840
#define CM_CHANNELPANEL_CURCHANNEL              841
#define CM_CHANNELPANEL_INFOPOPUP               842
#define CM_CHANNELPANEL_DETAILPOPUP             843
#define CM_CHANNELPANEL_SCROLLTOCURCHANNEL      844
#define CM_CHANNELPANEL_EVENTS_1                845
#define CM_CHANNELPANEL_EVENTS_2                (CM_CHANNELPANEL_EVENTS_1 + 1)
#define CM_CHANNELPANEL_EVENTS_3                (CM_CHANNELPANEL_EVENTS_1 + 2)
#define CM_CHANNELPANEL_EVENTS_4                (CM_CHANNELPANEL_EVENTS_1 + 3)
#define CM_CHANNELPANEL_EXPANDEVENTS_2          849
#define CM_CHANNELPANEL_EXPANDEVENTS_3          (CM_CHANNELPANEL_EXPANDEVENTS_2 + 1)
#define CM_CHANNELPANEL_EXPANDEVENTS_4          (CM_CHANNELPANEL_EXPANDEVENTS_2 + 2)
#define CM_CHANNELPANEL_EXPANDEVENTS_5          (CM_CHANNELPANEL_EXPANDEVENTS_2 + 3)
#define CM_CHANNELPANEL_EXPANDEVENTS_6          (CM_CHANNELPANEL_EXPANDEVENTS_2 + 4)
#define CM_CHANNELPANEL_EXPANDEVENTS_7          (CM_CHANNELPANEL_EXPANDEVENTS_2 + 5)
#define CM_CHANNELPANEL_EXPANDEVENTS_8          (CM_CHANNELPANEL_EXPANDEVENTS_2 + 6)
#define CM_CHANNELPANEL_USEEPGCOLORSCHEME       856
#define CM_CHANNELPANEL_SHOWGENRECOLOR          857
#define CM_CHANNELPANEL_SHOWFEATUREDMARK        858
#define CM_CHANNELPANEL_PROGRESSBAR_NONE        859
#define CM_CHANNELPANEL_PROGRESSBAR_ELAPSED     860
#define CM_CHANNELPANEL_PROGRESSBAR_REMAINING   861

#define CM_CAPTIONPANEL_COPY                    860
#define CM_CAPTIONPANEL_SELECTALL               861
#define CM_CAPTIONPANEL_CLEAR                   862
#define CM_CAPTIONPANEL_SAVE                    863
#define CM_CAPTIONPANEL_ENABLE                  864
#define CM_CAPTIONPANEL_AUTOSCROLL              865
#define CM_CAPTIONPANEL_IGNORESMALL             866
#define CM_CAPTIONPANEL_HALFWIDTHALNUM          867
#define CM_CAPTIONPANEL_HALFWIDTHEUROLANGS      868
#define CM_CAPTIONPANEL_LANGUAGE_FIRST          869
#define CM_CAPTIONPANEL_LANGUAGE_LAST           (CM_CAPTIONPANEL_LANGUAGE_FIRST + 7)

#define CM_PROGRAMLISTPANEL_MOUSEOVEREVENTINFO  880
#define CM_PROGRAMLISTPANEL_USEEPGCOLORSCHEME   881
#define CM_PROGRAMLISTPANEL_SHOWFEATUREDMARK    882

#define CM_PROGRAMGUIDE_UPDATE                  2000
#define CM_PROGRAMGUIDE_ENDUPDATE               2001
#define CM_PROGRAMGUIDE_REFRESH                 2002
#define CM_PROGRAMGUIDE_AUTOREFRESH             2003
#define CM_PROGRAMGUIDE_SEARCH                  2004
#define CM_PROGRAMGUIDE_CHANNELSETTINGS         2005
#define CM_PROGRAMGUIDE_ALWAYSONTOP             2006
#define CM_PROGRAMGUIDE_DRAGSCROLL              2007
#define CM_PROGRAMGUIDE_POPUPEVENTINFO          2008
#define CM_PROGRAMGUIDE_KEEPTIMEPOS             2009
#define CM_PROGRAMGUIDE_IEPGASSOCIATE           2010
#define CM_PROGRAMGUIDE_JUMPEVENT               2011
#define CM_PROGRAMGUIDE_TOOLBAR_TUNERMENU       2012
#define CM_PROGRAMGUIDE_TOOLBAR_DATEMENU        2013
#define CM_PROGRAMGUIDE_TOOLBAR_FAVORITES       2014
#define CM_PROGRAMGUIDE_TOOLBAR_DATE            2015
#define CM_PROGRAMGUIDE_TOOLBAR_TIME            2016
#define CM_PROGRAMGUIDE_TOOLBAROPTIONS          2017
#define CM_PROGRAMGUIDE_ADDTOFAVORITES          2018
#define CM_PROGRAMGUIDE_ORGANIZEFAVORITES       2019
#define CM_PROGRAMGUIDE_SHOWFEATUREDMARK        2020
#define CM_PROGRAMGUIDE_DAY_FIRST               2030
#define CM_PROGRAMGUIDE_DAY_LAST                2037
#define CM_PROGRAMGUIDE_DAY_TODAY               CM_PROGRAMGUIDE_DAY_FIRST
#define CM_PROGRAMGUIDE_TIME_CURRENT            2039
#define CM_PROGRAMGUIDE_TIME_FIRST              2040
#define CM_PROGRAMGUIDE_TIME_LAST               2059
#define CM_PROGRAMGUIDE_FILTER_FIRST            2060
#define CM_PROGRAMGUIDE_FILTER_FREE             (CM_PROGRAMGUIDE_FILTER_FIRST + 0)
#define CM_PROGRAMGUIDE_FILTER_NEWPROGRAM       (CM_PROGRAMGUIDE_FILTER_FIRST + 1)
#define CM_PROGRAMGUIDE_FILTER_ORIGINAL         (CM_PROGRAMGUIDE_FILTER_FIRST + 2)
#define CM_PROGRAMGUIDE_FILTER_RERUN            (CM_PROGRAMGUIDE_FILTER_FIRST + 3)
#define CM_PROGRAMGUIDE_FILTER_NOT_SHOPPING     (CM_PROGRAMGUIDE_FILTER_FIRST + 4)
#define CM_PROGRAMGUIDE_FILTER_NEWS             (CM_PROGRAMGUIDE_FILTER_FIRST + 8)
#define CM_PROGRAMGUIDE_FILTER_SPORTS           (CM_PROGRAMGUIDE_FILTER_FIRST + 9)
#define CM_PROGRAMGUIDE_FILTER_INFORMATION      (CM_PROGRAMGUIDE_FILTER_FIRST + 10)
#define CM_PROGRAMGUIDE_FILTER_DRAMA            (CM_PROGRAMGUIDE_FILTER_FIRST + 11)
#define CM_PROGRAMGUIDE_FILTER_MUSIC            (CM_PROGRAMGUIDE_FILTER_FIRST + 12)
#define CM_PROGRAMGUIDE_FILTER_VARIETY          (CM_PROGRAMGUIDE_FILTER_FIRST + 13)
#define CM_PROGRAMGUIDE_FILTER_MOVIE            (CM_PROGRAMGUIDE_FILTER_FIRST + 14)
#define CM_PROGRAMGUIDE_FILTER_ANIME            (CM_PROGRAMGUIDE_FILTER_FIRST + 15)
#define CM_PROGRAMGUIDE_FILTER_DOCUMENTARY      (CM_PROGRAMGUIDE_FILTER_FIRST + 16)
#define CM_PROGRAMGUIDE_FILTER_THEATER          (CM_PROGRAMGUIDE_FILTER_FIRST + 17)
#define CM_PROGRAMGUIDE_FILTER_EDUCATION        (CM_PROGRAMGUIDE_FILTER_FIRST + 18)
#define CM_PROGRAMGUIDE_FILTER_WELFARE          (CM_PROGRAMGUIDE_FILTER_FIRST + 19)
#define CM_PROGRAMGUIDE_FILTER_LAST             (CM_PROGRAMGUIDE_FILTER_FIRST + 19)
#define CM_PROGRAMGUIDE_CHANNELGROUP_FIRST      2100
#define CM_PROGRAMGUIDE_CHANNELGROUP_LAST       2199
#define CM_PROGRAMGUIDE_CHANNELPROVIDER_FIRST   2200
#define CM_PROGRAMGUIDE_CHANNELPROVIDER_LAST    2299
#define CM_PROGRAMGUIDE_FAVORITE_FIRST          2300
#define CM_PROGRAMGUIDE_FAVORITE_LAST           2399

#define CM_FEATUREDEVENTS_SETTINGS              2800
#define CM_FEATUREDEVENTS_SHOWEVENTTEXT         2801
#define CM_FEATUREDEVENTS_SORT_FIRST            2810
#define CM_FEATUREDEVENTS_SORT_TIME             (CM_FEATUREDEVENTS_SORT_FIRST + 0)
#define CM_FEATUREDEVENTS_SORT_SERVICE          (CM_FEATUREDEVENTS_SORT_FIRST + 1)
#define CM_FEATUREDEVENTS_SORT_LAST             (CM_FEATUREDEVENTS_SORT_FIRST + 1)

#define CM_PLUGIN_FIRST                         3000
#define CM_PLUGIN_LAST                          3999

#define CM_CHANNEL_FIRST                        4000
#define CM_CHANNEL_LAST                         4999

#define CM_SERVICE_FIRST                        5000
#define CM_SERVICE_LAST                         5999

#define CM_SPACE_ALL                            6000
#define CM_SPACE_FIRST                          6001
#define CM_SPACE_LAST                           6099

#define CM_DRIVER_FIRST                         6100
#define CM_DRIVER_LAST                          6198
#define CM_DRIVER_BROWSE                        6199

#define CM_SPACE_CHANNEL_FIRST                  7000
#define CM_SPACE_CHANNEL_LAST                   11999

#define CM_CHANNELNO_FIRST                      12000
#define CM_CHANNELNO_LAST                       (CM_CHANNELNO_FIRST + 999)
#define CM_CHANNELNO_1                          (CM_CHANNELNO_FIRST + 0)
#define CM_CHANNELNO_2                          (CM_CHANNELNO_FIRST + 1)
#define CM_CHANNELNO_3                          (CM_CHANNELNO_FIRST + 2)
#define CM_CHANNELNO_4                          (CM_CHANNELNO_FIRST + 3)
#define CM_CHANNELNO_5                          (CM_CHANNELNO_FIRST + 4)
#define CM_CHANNELNO_6                          (CM_CHANNELNO_FIRST + 5)
#define CM_CHANNELNO_7                          (CM_CHANNELNO_FIRST + 6)
#define CM_CHANNELNO_8                          (CM_CHANNELNO_FIRST + 7)
#define CM_CHANNELNO_9                          (CM_CHANNELNO_FIRST + 8)
#define CM_CHANNELNO_10                         (CM_CHANNELNO_FIRST + 9)
#define CM_CHANNELNO_11                         (CM_CHANNELNO_FIRST + 10)
#define CM_CHANNELNO_12                         (CM_CHANNELNO_FIRST + 11)

#define CM_CHANNELHISTORY_FIRST                 13000
#define CM_CHANNELHISTORY_LAST                  13098
#define CM_CHANNELHISTORY_CLEAR                 13099

#define CM_CHANNELNO_2DIGIT                     13100
#define CM_CHANNELNO_3DIGIT                     13101

#define CM_ADDTOFAVORITES                       13200
#define CM_ORGANIZEFAVORITES                    13201
#define CM_FAVORITESSUBMENU                     13209
#define CM_FAVORITECHANNEL_FIRST                14000
#define CM_FAVORITECHANNEL_LAST                 14999

#define CM_PLUGINCOMMAND_FIRST                  15000
#define CM_PLUGINCOMMAND_LAST                   15999

#define CM_PROGRAMGUIDE_CUSTOM_FIRST            16000
#define CM_PROGRAMGUIDE_CUSTOM_LAST             16999

#define CM_PROGRAMGUIDETOOL_FIRST               17000
#define CM_PROGRAMGUIDETOOL_LAST                17999

#define CM_PANANDSCAN_PRESET_FIRST              18000
#define CM_PANANDSCAN_PRESET_LAST               18999

#define CM_CUSTOMZOOM_FIRST                     19000
#define CM_CUSTOMZOOM_LAST                      19009

#define CM_SWITCHVIDEO                          19100
#define CM_VIDEOSTREAM_SWITCH                   19101
#define CM_VIDEOSTREAM_FIRST                    19110
#define CM_VIDEOSTREAM_LAST                     (CM_VIDEOSTREAM_FIRST + 31)

#define CM_AUDIOSTREAM_FIRST                    19150
#define CM_AUDIOSTREAM_LAST                     19169
#define CM_AUDIO_FIRST                          19170
#define CM_AUDIO_LAST                           19189

#define CM_MULTIVIEW_SWITCH                     19200
#define CM_MULTIVIEW_FIRST                      19210
#define CM_MULTIVIEW_LAST                       (CM_MULTIVIEW_FIRST + 15)

#define CM_PANEL_FIRST                          19300
#define CM_PANEL_LAST                           19399
#define CM_PANEL_INFORMATION                    (CM_PANEL_FIRST + 0)
#define CM_PANEL_PROGRAMLIST                    (CM_PANEL_FIRST + 1)
#define CM_PANEL_CHANNEL                        (CM_PANEL_FIRST + 2)
#define CM_PANEL_CONTROL                        (CM_PANEL_FIRST + 3)
#define CM_PANEL_CAPTION                        (CM_PANEL_FIRST + 4)

#define CM_WHEEL_VOLUME                         19400
#define CM_WHEEL_CHANNEL                        19401
#define CM_WHEEL_AUDIO                          19402
#define CM_WHEEL_ZOOM                           19403
#define CM_WHEEL_ASPECTRATIO                    19404
#define CM_WHEEL_AUDIODELAY                     19405

#define CM_COMMAND_LAST                         19405

#define IDS_MENU_ZOOM              20000
#define IDS_MENU_ASPECTRATIO       20001
#define IDS_MENU_CHANNEL           20002
#define IDS_MENU_SERVICE           20003
#define IDS_MENU_TUNER             20004
#define IDS_MENU_FAVORITES         20005
#define IDS_MENU_CHANNELHISTORY    20006
#define IDS_MENU_VOLUME            20007
#define IDS_MENU_AUDIO             20008
#define IDS_MENU_VIDEO             20009
#define IDS_MENU_RESET             20010
#define IDS_MENU_BAR               20011
#define IDS_MENU_PLUGIN            20012
#define IDS_MENU_FILTERPROPERTY    20013

#define IDS_MENU_ADDTOFAVORITES    20100
#define IDS_MENU_ORGANIZEFAVORITES 20101

#define IDS_POWERREQUEST           20200

#define SC_ABOUT   0x1000
#define SC_DOCKING 0x1001

#define IDC_VIEW            1000
#define IDC_VIDEOCONTAINER  1001
#define IDC_STATUS          1002
#define IDC_TITLEBAR        1003
#define IDC_SIDEBAR         1004

#define IDC_ABOUT_LOGO   1000
#define IDC_ABOUT_HEADER 1001
#define IDC_ABOUT_INFO   1002
#define IDC_ABOUT_LINK   1003

#define IDC_ERROR_ICON    1000
#define IDC_ERROR_MESSAGE 1001
#define IDC_ERROR_COPY    1002

#define IDC_INITIALSETTINGS_LOGO                1000
#define IDC_INITIALSETTINGS_DRIVER              1001
#define IDC_INITIALSETTINGS_DRIVER_BROWSE       1002
#define IDC_INITIALSETTINGS_MPEG2DECODER        1003
#define IDC_INITIALSETTINGS_H264DECODER         1004
#define IDC_INITIALSETTINGS_H265DECODER         1005
#define IDC_INITIALSETTINGS_VIDEORENDERER       1006
#define IDC_INITIALSETTINGS_RECORDFOLDER        1007
#define IDC_INITIALSETTINGS_RECORDFOLDER_BROWSE 1008
#define IDC_INITIALSETTINGS_HELP                1009

#define IDC_OPTIONS_LIST      1000
#define IDC_OPTIONS_TITLE     1001
#define IDC_OPTIONS_PAGEPLACE 1002
#define IDC_OPTIONS_HELP      1003
#define IDC_OPTIONS_SEPARATOR 1004

#define IDC_OPTIONS_DRIVERDIRECTORY        1000
#define IDC_OPTIONS_DRIVERDIRECTORY_BROWSE 1001
#define IDC_OPTIONS_DEFAULTDRIVER_NONE     1002
#define IDC_OPTIONS_DEFAULTDRIVER_LAST     1003
#define IDC_OPTIONS_DEFAULTDRIVER_CUSTOM   1004
#define IDC_OPTIONS_DEFAULTDRIVER          1005
#define IDC_OPTIONS_DEFAULTDRIVER_BROWSE   1006
#define IDC_OPTIONS_RESIDENT               1007
#define IDC_OPTIONS_KEEPSINGLETASK         1008
#define IDC_OPTIONS_STANDALONEPROGRAMGUIDE 1009
#define IDC_OPTIONS_ENABLE1SEGFALLBACK     1010
#define IDC_OPTIONS_ENABLEJUMPLIST         1011
#define IDC_OPTIONS_JUMPLISTKEEPSINGLETASK 1012
#define IDC_OPTIONS_USEUNIQUEAPPID         1013

#define IDC_OPTIONS_SNAPATWINDOWEDGE            1000
#define IDC_OPTIONS_SUPPORTAEROSNAP             1001
#define IDC_OPTIONS_ADJUSTASPECTRESIZING        1002
#define IDC_OPTIONS_NEARCORNERRESIZEORIGIN      1003
#define IDC_OPTIONS_PANSCANADJUSTWINDOW         1004
#define IDC_OPTIONS_ZOOMKEEPASPECTRATIO         1005
#define IDC_OPTIONS_REMEMBER1SEGWINDOWSIZE      1006
#define IDC_OPTIONS_MINIMIZETOTRAY              1007
#define IDC_OPTIONS_MINIMIZEDISABLEPREVIEW      1008
#define IDC_OPTIONS_HIDECURSOR                  1009
#define IDC_OPTIONS_USELOGOICON                 1010
#define IDC_OPTIONS_TITLETEXTFORMAT             1011
#define IDC_OPTIONS_TITLETEXTFORMAT_PARAMETERS  1012
#define IDC_OPTIONS_TITLETEXTFORMAT_PRESETS     1013
#define IDC_OPTIONS_TITLEBARFONT_ENABLE         1014
#define IDC_OPTIONS_TITLEBARFONT_INFO           1015
#define IDC_OPTIONS_TITLEBARFONT_CHOOSE         1016
#define IDC_OPTIONS_SHOWLOGO                    1017
#define IDC_OPTIONS_LOGOFILENAME                1018
#define IDC_OPTIONS_LOGOFILENAME_BROWSE         1019
#define IDC_OPTIONS_NOSCREENSAVER               1020
#define IDC_OPTIONS_NOMONITORLOWPOWER           1021
#define IDC_OPTIONS_NOMONITORLOWPOWERACTIVEONLY 1022
#define IDC_OPTIONS_WINDOW_SEPARATOR            1023
#define IDC_OPTIONS_PREVENT_SEPARATOR           1024

#define IDC_OSDOPTIONS_GROUP                       1000
#define IDC_OSDOPTIONS_SHOWOSD                     1001
#define IDC_OSDOPTIONS_COMPOSITE                   1002
#define IDC_OSDOPTIONS_TEXTCOLOR_LABEL             1003
#define IDC_OSDOPTIONS_TEXTCOLOR                   1004
#define IDC_OSDOPTIONS_OSDFONT_LABEL               1005
#define IDC_OSDOPTIONS_OSDFONT_INFO                1006
#define IDC_OSDOPTIONS_OSDFONT_CHOOSE              1007
#define IDC_OSDOPTIONS_FADETIME_LABEL              1008
#define IDC_OSDOPTIONS_FADETIME                    1009
#define IDC_OSDOPTIONS_FADETIME_UD                 1010
#define IDC_OSDOPTIONS_FADETIME_UNIT               1011
#define IDC_OSDOPTIONS_SHOW_LABEL                  1012
#define IDC_OSDOPTIONS_SHOW_CHANNEL                1013
#define IDC_OSDOPTIONS_SHOW_VOLUME                 1014
#define IDC_OSDOPTIONS_SHOW_AUDIO                  1015
#define IDC_OSDOPTIONS_SHOW_RECORDING              1016
#define IDC_OSDOPTIONS_SHOW_CHANNELNOINPUT         1017
#define IDC_OSDOPTIONS_CHANNELCHANGE_TYPE_LABEL    1018
#define IDC_OSDOPTIONS_CHANNELCHANGE_TYPE          1019
#define IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_LABEL    1020
#define IDC_OSDOPTIONS_CHANNELCHANGE_TEXT          1021
#define IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_PARAMS   1022
#define IDC_OSDOPTIONS_FIRST                       IDC_OSDOPTIONS_COMPOSITE
#define IDC_OSDOPTIONS_LAST                        IDC_OSDOPTIONS_CHANNELCHANGE_TEXT_PARAMS
#define IDC_NOTIFICATIONBAR_ENABLE                 1030
#define IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME        1031
#define IDC_NOTIFICATIONBAR_NOTIFYTSPROCESSORERROR 1032
#define IDC_NOTIFICATIONBAR_DURATION_LABEL         1033
#define IDC_NOTIFICATIONBAR_DURATION               1034
#define IDC_NOTIFICATIONBAR_DURATION_UPDOWN        1035
#define IDC_NOTIFICATIONBAR_DURATION_UNIT          1036
#define IDC_NOTIFICATIONBAR_FONT_LABEL             1037
#define IDC_NOTIFICATIONBAR_FONT_INFO              1038
#define IDC_NOTIFICATIONBAR_FONT_CHOOSE            1039
#define IDC_NOTIFICATIONBAR_FIRST                  IDC_NOTIFICATIONBAR_NOTIFYEVENTNAME
#define IDC_NOTIFICATIONBAR_LAST                   IDC_NOTIFICATIONBAR_FONT_CHOOSE
#define IDC_DISPLAYMENU_FONT_LABEL                 1050
#define IDC_DISPLAYMENU_FONT_INFO                  1051
#define IDC_DISPLAYMENU_FONT_CHOOSE                1052
#define IDC_DISPLAYMENU_AUTOFONTSIZE               1053

#define IDC_STATUSOPTIONS_ITEMLIST       1000
#define IDC_STATUSOPTIONS_DEFAULT        1001
#define IDC_STATUSOPTIONS_FONTINFO_LABEL 1010
#define IDC_STATUSOPTIONS_FONTINFO       1011
#define IDC_STATUSOPTIONS_CHOOSEFONT     1012
#define IDC_STATUSOPTIONS_MULTIROW       1013
#define IDC_STATUSOPTIONS_MAXROWS_LABEL  1014
#define IDC_STATUSOPTIONS_MAXROWS        1015
#define IDC_STATUSOPTIONS_MAXROWS_UPDOWN 1016
#define IDC_STATUSOPTIONS_SHOWPOPUP      1017
#define IDC_STATUSOPTIONS_OPACITY_LABEL  1018
#define IDC_STATUSOPTIONS_OPACITY_SLIDER 1019
#define IDC_STATUSOPTIONS_OPACITY_INPUT  1020
#define IDC_STATUSOPTIONS_OPACITY_SPIN   1021
#define IDC_STATUSOPTIONS_OPACITY_UNIT   1022

#define IDC_SIDEBAR_SHOWPOPUP         1000
#define IDC_SIDEBAR_OPACITY_LABEL     1001
#define IDC_SIDEBAR_OPACITY_SLIDER    1002
#define IDC_SIDEBAR_OPACITY_INPUT     1003
#define IDC_SIDEBAR_OPACITY_SPIN      1004
#define IDC_SIDEBAR_OPACITY_UNIT      1005
#define IDC_SIDEBAR_SHOWTOOLTIPS      1006
#define IDC_SIDEBAR_SHOWCHANNELLOGO   1007
#define IDC_SIDEBAR_ITEMLIST_LABEL    1008
#define IDC_SIDEBAR_ITEMLIST          1009
#define IDC_SIDEBAR_UP                1010
#define IDC_SIDEBAR_DOWN              1011
#define IDC_SIDEBAR_REMOVE            1012
#define IDC_SIDEBAR_DEFAULT           1013
#define IDC_SIDEBAR_COMMANDLIST_LABEL 1014
#define IDC_SIDEBAR_COMMANDLIST       1015
#define IDC_SIDEBAR_ADD               1016
#define IDC_SIDEBAR_SEPARATOR         1017

#define IDC_MENUOPTIONS_MAXCHANNELMENUROWS           1000
#define IDC_MENUOPTIONS_MAXCHANNELMENUROWS_SPIN      1001
#define IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO      1002
#define IDC_MENUOPTIONS_MAXCHANNELMENUEVENTINFO_SPIN 1003
#define IDC_MENUOPTIONS_ITEMLIST                     1010
#define IDC_MENUOPTIONS_ITEMLIST_UP                  1011
#define IDC_MENUOPTIONS_ITEMLIST_DOWN                1012
#define IDC_MENUOPTIONS_ITEMLIST_INSERTSEPARATOR     1013
#define IDC_MENUOPTIONS_ITEMLIST_REMOVESEPARATOR     1014
#define IDC_MENUOPTIONS_ITEMLIST_DEFAULT             1015

#define IDC_PANELOPTIONS_SNAPATMAINWINDOW   1000
#define IDC_PANELOPTIONS_ATTACHTOMAINWINDOW 1001
#define IDC_PANELOPTIONS_OPACITY_TB         1002
#define IDC_PANELOPTIONS_OPACITY_EDIT       1003
#define IDC_PANELOPTIONS_OPACITY_UD         1004
#define IDC_PANELOPTIONS_FONTINFO           1005
#define IDC_PANELOPTIONS_CHOOSEFONT         1006
#define IDC_PANELOPTIONS_SPECCAPTIONFONT    1007
#define IDC_PANELOPTIONS_CAPTIONFONT_INFO   1008
#define IDC_PANELOPTIONS_CAPTIONFONT_CHOOSE 1009
#define IDC_PANELOPTIONS_ITEMLIST           1010
#define IDC_PANELOPTIONS_ITEMLIST_UP        1011
#define IDC_PANELOPTIONS_ITEMLIST_DOWN      1012
#define IDC_PANELOPTIONS_FIRSTTAB           1013
#define IDC_PANELOPTIONS_TABSTYLE           1014
#define IDC_PANELOPTIONS_TABTOOLTIP         1015

#define IDC_COLORSCHEME_PRESET               1000
#define IDC_COLORSCHEME_SAVE                 1001
#define IDC_COLORSCHEME_DELETE               1002
#define IDC_COLORSCHEME_LIST                 1003
#define IDC_COLORSCHEME_PALETTEPLACE         1004
#define IDC_COLORSCHEME_PALETTE              1005
#define IDC_COLORSCHEME_DEFAULT              1006
#define IDC_COLORSCHEME_PREVIEW              1007
#define IDC_COLORSCHEME_SELECTSAMECOLOR      1010
#define IDC_COLORSCHEME_GRADIENT_NORMAL      1011
#define IDC_COLORSCHEME_GRADIENT_GLOSSY      1012
#define IDC_COLORSCHEME_GRADIENT_INTERLACED  1013
#define IDC_COLORSCHEME_DIRECTION_HORZ       1014
#define IDC_COLORSCHEME_DIRECTION_VERT       1015
#define IDC_COLORSCHEME_DIRECTION_HORZMIRROR 1016
#define IDC_COLORSCHEME_DIRECTION_VERTMIRROR 1017
#define IDC_COLORSCHEME_BORDER_NONE          1018
#define IDC_COLORSCHEME_BORDER_SOLID         1019
#define IDC_COLORSCHEME_BORDER_SUNKEN        1020
#define IDC_COLORSCHEME_BORDER_RAISED        1021

#define IDC_SAVECOLORSCHEME_NAME 1000

#define IDC_OPTIONS_DISPLAYDRAGMOVE         1000
#define IDC_OPTIONS_WHEELMODE               1010
#define IDC_OPTIONS_WHEELSHIFTMODE          1011
#define IDC_OPTIONS_WHEELCTRLMODE           1012
#define IDC_OPTIONS_WHEELTILTMODE           1013
#define IDC_OPTIONS_WHEELVOLUMEREVERSE      1014
#define IDC_OPTIONS_WHEELCHANNELREVERSE     1015
#define IDC_OPTIONS_WHEELCHANNELDELAY       1016
#define IDC_OPTIONS_WHEELCHANNELMININTERVAL 1017
#define IDC_OPTIONS_STATUSBARWHEEL          1018
#define IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND  1020
#define IDC_OPTIONS_RIGHTCLICKCOMMAND       1021
#define IDC_OPTIONS_MIDDLECLICKCOMMAND      1022
#define IDC_OPTIONS_MOUSECOMMAND_FIRST      IDC_OPTIONS_LEFTDOUBLECLICKCOMMAND
#define IDC_OPTIONS_MOUSECOMMAND_LAST       IDC_OPTIONS_MIDDLECLICKCOMMAND
#define IDC_OPTIONS_VOLUMESTEP              1030
#define IDC_OPTIONS_VOLUMESTEP_UD           1031
#define IDC_OPTIONS_AUDIODELAYSTEP          1032
#define IDC_OPTIONS_AUDIODELAYSTEP_UD       1033
#define IDC_OPTIONS_CHANNELUPDOWNORDER      1034
#define IDC_OPTIONS_SKIPSUBCHANNEL          1035

#define IDC_ACCELERATOR_GROUP               1000
#define IDC_ACCELERATOR_LIST                1001
#define IDC_ACCELERATOR_KEY                 1002
#define IDC_ACCELERATOR_SHIFT               1003
#define IDC_ACCELERATOR_CONTROL             1004
#define IDC_ACCELERATOR_ALT                 1005
#define IDC_ACCELERATOR_GLOBAL              1006
#define IDC_ACCELERATOR_APPCOMMAND          1007
#define IDC_ACCELERATOR_NOTE                1008
#define IDC_ACCELERATOR_DEFAULT             1009
#define IDC_ACCELERATOR_CHANNELINPUTOPTIONS 1010

#define IDC_CONTROLLER_LIST         1000
#define IDC_CONTROLLER_ACTIVEONLY   1001
#define IDC_CONTROLLER_ASSIGN_LABEL 1002
#define IDC_CONTROLLER_ASSIGN       1003
#define IDC_CONTROLLER_DEFAULT      1004
#define IDC_CONTROLLER_COMMAND      1005
#define IDC_CONTROLLER_IMAGEPLACE   1006

#define IDC_DRIVEROPTIONS_DRIVERLIST                     1000
#define IDC_DRIVEROPTIONS_INITCHANNEL_GROUP              1001
#define IDC_DRIVEROPTIONS_INITCHANNEL_NONE               1002
#define IDC_DRIVEROPTIONS_INITCHANNEL_LAST               1003
#define IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM             1004
#define IDC_DRIVEROPTIONS_INITCHANNEL_SPACE              1005
#define IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL            1006
#define IDC_DRIVEROPTIONS_NOSIGNALLEVEL                  1008
#define IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM            1009
#define IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE     1010
#define IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT   1011
#define IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK         1012
#define IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_LABEL     1013
#define IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY           1014
#define IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_SPIN      1015
#define IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_UNIT      1016
#define IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_LABEL 1017
#define IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL       1018
#define IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_SPIN  1019
#define IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_UNIT  1020
#define IDC_DRIVEROPTIONS_FIRST                          IDC_DRIVEROPTIONS_INITCHANNEL_GROUP
#define IDC_DRIVEROPTIONS_LAST                           IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_UNIT

#define IDC_OPTIONS_MPEG2DECODER            1000
#define IDC_OPTIONS_H264DECODER             1001
#define IDC_OPTIONS_H265DECODER             1002
#define IDC_OPTIONS_RENDERER                1003
#define IDC_OPTIONS_RESETPANSCANEVENTCHANGE 1004
#define IDC_OPTIONS_NOMASKSIDECUT           1005
#define IDC_OPTIONS_FULLSCREENCUTFRAME      1006
#define IDC_OPTIONS_MAXIMIZECUTFRAME        1007
#define IDC_OPTIONS_IGNOREDISPLAYSIZE       1008
#define IDC_OPTIONS_CLIPTODEVICE            1009

#define IDC_OPTIONS_AUDIODEVICE                  1000
#define IDC_OPTIONS_AUDIOFILTER                  1001
#define IDC_OPTIONS_DOWNMIXSURROUND              1002
#define IDC_OPTIONS_SPDIFMODE                    1003
#define IDC_OPTIONS_SPDIF_CHANNELS_LABEL         1004
#define IDC_OPTIONS_SPDIF_CHANNELS_MONO          1005
#define IDC_OPTIONS_SPDIF_CHANNELS_DUALMONO      1006
#define IDC_OPTIONS_SPDIF_CHANNELS_STEREO        1007
#define IDC_OPTIONS_SPDIF_CHANNELS_SURROUND      1008
#define IDC_OPTIONS_SURROUNDOPTIONS              1009
#define IDC_OPTIONS_ENABLEAUDIOLANGUAGEPRIORITY  1010
#define IDC_OPTIONS_AUDIOLANGUAGEPRIORITY        1011
#define IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_ADD    1012
#define IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_REMOVE 1013
#define IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_UP     1014
#define IDC_OPTIONS_AUDIOLANGUAGEPRIORITY_DOWN   1015
#define IDC_OPTIONS_AUDIOLANGUAGELIST            1016

#define IDC_OPTIONS_RESTOREMUTE                1000
#define IDC_OPTIONS_RESTOREPLAYSTATUS          1001
#define IDC_OPTIONS_RESTORE1SEGMODE            1002
#define IDC_OPTIONS_MINTIMERRESOLUTION         1010
#define IDC_OPTIONS_ADJUSTAUDIOSTREAMTIME      1011
#define IDC_OPTIONS_PTSSYNC                    1012
#define IDC_OPTIONS_USEDEMUXERCLOCK            1013
#define IDC_OPTIONS_BUFFERSIZE_LABEL           1020
#define IDC_OPTIONS_BUFFERSIZE                 1021
#define IDC_OPTIONS_BUFFERSIZE_UD              1022
#define IDC_OPTIONS_BUFFERSIZE_UNIT            1023
#define IDC_OPTIONS_ENABLEBUFFERING            1024
#define IDC_OPTIONS_BUFFERPOOLPERCENTAGE_LABEL 1025
#define IDC_OPTIONS_BUFFERPOOLPERCENTAGE       1026
#define IDC_OPTIONS_BUFFERPOOLPERCENTAGE_UD    1027
#define IDC_OPTIONS_BUFFERPOOLPERCENTAGE_UNIT  1028
#define IDC_OPTIONS_BUFFERING_FIRST            IDC_OPTIONS_BUFFERPOOLPERCENTAGE_LABEL
#define IDC_OPTIONS_BUFFERING_LAST             IDC_OPTIONS_BUFFERPOOLPERCENTAGE_UNIT
#define IDC_OPTIONS_STREAMTHREADPRIORITY       1030
#define IDC_OPTIONS_ADJUSTFRAMERATE            1031

#define IDC_RECORDOPTIONS_SAVEFOLDER                  1000
#define IDC_RECORDOPTIONS_SAVEFOLDER_BROWSE           1001
#define IDC_RECORDOPTIONS_FILENAME                    1002
#define IDC_RECORDOPTIONS_FILENAMEFORMAT              1003
#define IDC_RECORDOPTIONS_FILENAMEPREVIEW             1004
#define IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE        1005
#define IDC_RECORDOPTIONS_CONFIRMEXIT                 1006
#define IDC_RECORDOPTIONS_CONFIRMSTOP                 1007
#define IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP        1008
#define IDC_RECORDOPTIONS_ALERTLOWFREESPACE           1009
#define IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL 1010
#define IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD       1011
#define IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_SPIN  1012
#define IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT  1013
#define IDC_RECORDOPTIONS_STATUSBARCOMMAND            1014
#define IDC_RECORDOPTIONS_CURSERVICEONLY              1015
#define IDC_RECORDOPTIONS_SAVESUBTITLE                1016
#define IDC_RECORDOPTIONS_SAVEDATACARROUSEL           1017
#define IDC_RECORDOPTIONS_WRITEPLUGIN                 1018
#define IDC_RECORDOPTIONS_WRITEPLUGINSETTING          1019
#define IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE         1020
#define IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE_SPIN    1021
#define IDC_RECORDOPTIONS_BUFFERSIZE                  1022
#define IDC_RECORDOPTIONS_BUFFERSIZE_UD               1023
#define IDC_RECORDOPTIONS_MAXPENDINGSIZE              1024
#define IDC_RECORDOPTIONS_MAXPENDINGSIZE_SPIN         1025

#define IDC_CAPTUREOPTIONS_SAVEFOLDER          1000
#define IDC_CAPTUREOPTIONS_SAVEFOLDER_BROWSE   1001
#define IDC_CAPTUREOPTIONS_FILENAME            1002
#define IDC_CAPTUREOPTIONS_FILENAME_PARAMETERS 1003
#define IDC_CAPTUREOPTIONS_FILENAME_PREVIEW    1004
#define IDC_CAPTUREOPTIONS_SIZE                1005
#define IDC_CAPTUREOPTIONS_FORMAT              1006
#define IDC_CAPTUREOPTIONS_JPEGQUALITY_TB      1007
#define IDC_CAPTUREOPTIONS_JPEGQUALITY_EDIT    1008
#define IDC_CAPTUREOPTIONS_JPEGQUALITY_UD      1009
#define IDC_CAPTUREOPTIONS_PNGLEVEL_TB         1010
#define IDC_CAPTUREOPTIONS_PNGLEVEL_EDIT       1011
#define IDC_CAPTUREOPTIONS_PNGLEVEL_UD         1012
#define IDC_CAPTUREOPTIONS_ICONSAVEFILE        1013
#define IDC_CAPTUREOPTIONS_SETCOMMENT          1014
#define IDC_CAPTUREOPTIONS_COMMENT             1015
#define IDC_CAPTUREOPTIONS_COMMENT_PARAMETERS  1016

#define IDC_CHANNELSCAN_SPACE_LABEL       1000
#define IDC_CHANNELSCAN_SPACE             1001
#define IDC_CHANNELSCAN_CHANNELLIST       1002
#define IDC_CHANNELSCAN_CHANNELLIST_LABEL 1003
#define IDC_CHANNELSCAN_SCANSERVICE       1004
#define IDC_CHANNELSCAN_IGNORESIGNALLEVEL 1005
#define IDC_CHANNELSCAN_SETTINGS          1006
#define IDC_CHANNELSCAN_LOADPRESET        1007
#define IDC_CHANNELSCAN_START             1008
#define IDC_CHANNELSCAN_PROPERTIES        1009
#define IDC_CHANNELSCAN_DELETE            1010
#define IDC_CHANNELSCAN_FIRST             IDC_CHANNELSCAN_SPACE_LABEL
#define IDC_CHANNELSCAN_LAST              IDC_CHANNELSCAN_START

#define IDC_CHANNELSCAN_INFO     1000
#define IDC_CHANNELSCAN_PROGRESS 1001
#define IDC_CHANNELSCAN_CHANNEL  1002
#define IDC_CHANNELSCAN_LEVEL    1003
#define IDC_CHANNELSCAN_GRAPH    1004

#define IDC_CHANNELPROP_NAME         1000
#define IDC_CHANNELPROP_CONTROLKEY   1001
#define IDC_CHANNELPROP_NETWORKID    1002
#define IDC_CHANNELPROP_TSID         1003
#define IDC_CHANNELPROP_SERVICEID    1004
#define IDC_CHANNELPROP_TUNINGSPACE  1005
#define IDC_CHANNELPROP_CHANNELINDEX 1006

#define IDC_CHANNELSCANSETTINGS_SCANWAIT             1000
#define IDC_CHANNELSCANSETTINGS_RETRYCOUNT           1001
#define IDC_CHANNELSCANSETTINGS_SIGNALLEVELTHRESHOLD 1002
#define IDC_CHANNELSCANSETTINGS_DETECTDATASERVICE    1003
#define IDC_CHANNELSCANSETTINGS_DETECT1SEGSERVICE    1004
#define IDC_CHANNELSCANSETTINGS_DETECTAUDIOSERVICE   1005
#define IDC_CHANNELSCANSETTINGS_HELP                 1006

#define IDC_EPGOPTIONS_GROUP                1000
#define IDC_EPGOPTIONS_SAVEEPGFILE          1001
#define IDC_EPGOPTIONS_EPGFILENAME_LABEL    1002
#define IDC_EPGOPTIONS_EPGFILENAME          1003
#define IDC_EPGOPTIONS_EPGFILENAME_BROWSE   1004
#define IDC_EPGOPTIONS_UPDATEWHENSTANDBY    1005
#define IDC_EPGOPTIONS_UPDATEBSEXTENDED     1006
#define IDC_EPGOPTIONS_UPDATECSEXTENDED     1007
#define IDC_EPGOPTIONS_USEEPGDATA           1008
#define IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL  1009
#define IDC_EPGOPTIONS_EPGDATAFOLDER        1010
#define IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE 1011
#define IDC_LOGOOPTIONS_GROUP               1020
#define IDC_LOGOOPTIONS_SAVEDATA            1021
#define IDC_LOGOOPTIONS_DATAFILENAME_LABEL  1022
#define IDC_LOGOOPTIONS_DATAFILENAME        1023
#define IDC_LOGOOPTIONS_DATAFILENAME_BROWSE 1024
#define IDC_LOGOOPTIONS_SAVERAWLOGO         1025
#define IDC_LOGOOPTIONS_SAVEBMPLOGO         1026
#define IDC_LOGOOPTIONS_LOGOFOLDER          1027
#define IDC_LOGOOPTIONS_LOGOFOLDER_BROWSE   1028
#define IDC_EVENTINFOOPTIONS_GROUP          1030
#define IDC_EVENTINFOOPTIONS_FONT_INFO      1031
#define IDC_EVENTINFOOPTIONS_FONT_CHOOSE    1032

#define IDC_PROGRAMGUIDEOPTIONS_ONSCREEN            1000
#define IDC_PROGRAMGUIDEOPTIONS_SCROLLTOCURCHANNEL  1001
#define IDC_PROGRAMGUIDEOPTIONS_BEGINHOUR           1002
#define IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS           1003
#define IDC_PROGRAMGUIDEOPTIONS_VIEWHOURS_UD        1004
#define IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH        1005
#define IDC_PROGRAMGUIDEOPTIONS_CHANNELWIDTH_UD     1006
#define IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR        1007
#define IDC_PROGRAMGUIDEOPTIONS_LINESPERHOUR_UD     1008
#define IDC_PROGRAMGUIDEOPTIONS_FONTINFO            1011
#define IDC_PROGRAMGUIDEOPTIONS_CHOOSEFONT          1012
#define IDC_PROGRAMGUIDEOPTIONS_USEDIRECTWRITE      1013
#define IDC_PROGRAMGUIDEOPTIONS_DIRECTWRITEOPTIONS  1014
#define IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST          1020
#define IDC_PROGRAMGUIDEOPTIONS_ICON_HD             (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 0)
#define IDC_PROGRAMGUIDEOPTIONS_ICON_SD             (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 1)
#define IDC_PROGRAMGUIDEOPTIONS_ICON_5_1CH          (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 2)
#define IDC_PROGRAMGUIDEOPTIONS_ICON_MULTILINGUAL   (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 3)
#define IDC_PROGRAMGUIDEOPTIONS_ICON_SUB            (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 4)
#define IDC_PROGRAMGUIDEOPTIONS_ICON_FREE           (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 5)
#define IDC_PROGRAMGUIDEOPTIONS_ICON_PAY            (IDC_PROGRAMGUIDEOPTIONS_ICON_FIRST + 6)
#define IDC_PROGRAMGUIDEOPTIONS_USEARIBSYMBOL       1030
#define IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES    1031
#define IDC_PROGRAMGUIDEOPTIONS_WHEELSCROLLLINES_UD 1032
#define IDC_PROGRAMGUIDEOPTIONS_PROGRAMLDOUBLECLICK 1033
#define IDC_PROGRAMGUIDETOOL_GROUP                  1050
#define IDC_PROGRAMGUIDETOOL_LIST                   1051
#define IDC_PROGRAMGUIDETOOL_ADD                    1052
#define IDC_PROGRAMGUIDETOOL_EDIT                   1053
#define IDC_PROGRAMGUIDETOOL_UP                     1054
#define IDC_PROGRAMGUIDETOOL_DOWN                   1055
#define IDC_PROGRAMGUIDETOOL_REMOVE                 1056
#define IDC_PROGRAMGUIDETOOL_REMOVEALL              1057

#define IDC_PROGRAMGUIDETOOL_NAME      1000
#define IDC_PROGRAMGUIDETOOL_COMMAND   1001
#define IDC_PROGRAMGUIDETOOL_BROWSE    1002
#define IDC_PROGRAMGUIDETOOL_PARAMETER 1003
#define IDC_PROGRAMGUIDETOOL_HELP      1004

#define IDC_PLUGIN_LIST     1000
#define IDC_PLUGIN_SETTINGS 1001
#define IDC_PLUGIN_UNLOAD   1002

#define IDC_TSPROCESSOR_TSPROCESSORLIST     1000
#define IDC_TSPROCESSOR_PROPERTIES          1001
#define IDC_TSPROCESSOR_ENABLEPROCESSING    1002
#define IDC_TSPROCESSOR_DEFAULTMODULE_LABEL 1003
#define IDC_TSPROCESSOR_DEFAULTMODULE       1004
#define IDC_TSPROCESSOR_DEFAULTDEVICE_LABEL 1005
#define IDC_TSPROCESSOR_DEFAULTDEVICE       1006
#define IDC_TSPROCESSOR_DEFAULTFILTER_LABEL 1007
#define IDC_TSPROCESSOR_DEFAULTFILTER       1008
#define IDC_TSPROCESSOR_TUNERMAP_LABEL      1009
#define IDC_TSPROCESSOR_TUNERMAP            1010
#define IDC_TSPROCESSOR_TUNERMAP_ADD        1011
#define IDC_TSPROCESSOR_TUNERMAP_EDIT       1012
#define IDC_TSPROCESSOR_TUNERMAP_REMOVE     1013
#define IDC_TSPROCESSOR_TUNERMAP_UP         1014
#define IDC_TSPROCESSOR_TUNERMAP_DOWN       1015
#define IDC_TSPROCESSOR_SETTINGSFIRST       IDC_TSPROCESSOR_PROPERTIES
#define IDC_TSPROCESSOR_SETTINGSLAST        IDC_TSPROCESSOR_TUNERMAP_DOWN

#define IDC_TSPROCESSORTUNERMAP_TUNER             1000
#define IDC_TSPROCESSORTUNERMAP_NETWORKID         1001
#define IDC_TSPROCESSORTUNERMAP_TSID              1002
#define IDC_TSPROCESSORTUNERMAP_SERVICEID         1003
#define IDC_TSPROCESSORTUNERMAP_ENABLEPROCESSING  1004
#define IDC_TSPROCESSORTUNERMAP_DISABLEPROCESSING 1005
#define IDC_TSPROCESSORTUNERMAP_MODULE_LABEL      1006
#define IDC_TSPROCESSORTUNERMAP_MODULE            1007
#define IDC_TSPROCESSORTUNERMAP_DEVICE_LABEL      1008
#define IDC_TSPROCESSORTUNERMAP_DEVICE            1009
#define IDC_TSPROCESSORTUNERMAP_FILTER_LABEL      1010
#define IDC_TSPROCESSORTUNERMAP_FILTER            1011
#define IDC_TSPROCESSORTUNERMAP_HELP              1012

#define IDC_LOG_LIST          1000
#define IDC_LOG_OUTPUTTOFILE  1001
#define IDC_LOG_CLEAR         1002
#define IDC_LOG_COPY          1003
#define IDC_LOG_SAVE          1004

#define IDC_RECORD_FILENAME_LABEL         1000
#define IDC_RECORD_FILENAME               1001
#define IDC_RECORD_FILENAME_BROWSE        1002
#define IDC_RECORD_FILENAMEFORMAT         1003
#define IDC_RECORD_FILENAMEPREVIEW_LABEL  1004
#define IDC_RECORD_FILENAMEPREVIEW        1005
/*
#define IDC_RECORD_FILEEXISTS_LABEL       1003
#define IDC_RECORD_FILEEXISTS             1004
*/
#define IDC_RECORD_STARTTIME              1010
#define IDC_RECORD_START_NOW              1011
#define IDC_RECORD_START_DATETIME         1012
#define IDC_RECORD_START_DELAY            1013
#define IDC_RECORD_STARTTIME_TIME         1014
#define IDC_RECORD_STARTTIME_HOUR         1015
#define IDC_RECORD_STARTTIME_HOUR_UD      1016
#define IDC_RECORD_STARTTIME_HOUR_LABEL   1017
#define IDC_RECORD_STARTTIME_MINUTE       1018
#define IDC_RECORD_STARTTIME_MINUTE_UD    1019
#define IDC_RECORD_STARTTIME_MINUTE_LABEL 1020
#define IDC_RECORD_STARTTIME_SECOND       1021
#define IDC_RECORD_STARTTIME_SECOND_UD    1022
#define IDC_RECORD_STARTTIME_SECOND_LABEL 1023
#define IDC_RECORD_STOPSPECTIME           1030
#define IDC_RECORD_STOPDATETIME           1031
#define IDC_RECORD_STOPREMAINTIME         1032
#define IDC_RECORD_STOPTIME_TIME          1033
#define IDC_RECORD_STOPTIME_HOUR          1034
#define IDC_RECORD_STOPTIME_HOUR_UD       1035
#define IDC_RECORD_STOPTIME_HOUR_LABEL    1036
#define IDC_RECORD_STOPTIME_MINUTE        1037
#define IDC_RECORD_STOPTIME_MINUTE_UD     1038
#define IDC_RECORD_STOPTIME_MINUTE_LABEL  1039
#define IDC_RECORD_STOPTIME_SECOND        1040
#define IDC_RECORD_STOPTIME_SECOND_UD     1041
#define IDC_RECORD_STOPTIME_SECOND_LABEL  1042
#define IDC_RECORD_CURSERVICEONLY         1043
#define IDC_RECORD_SAVESUBTITLE           1044
#define IDC_RECORD_SAVEDATACARROUSEL      1045
#define IDC_RECORD_WRITEPLUGIN_LABEL      1046
#define IDC_RECORD_WRITEPLUGIN            1047
#define IDC_RECORD_WRITEPLUGINSETTING     1048
#define IDC_RECORD_CANCEL                 1050

#define IDC_EVENTSEARCH_KEYWORD              1000
#define IDC_EVENTSEARCH_CASESENSITIVE        1001
#define IDC_EVENTSEARCH_REGEXP               1002
#define IDC_EVENTSEARCH_KEYWORDTARGET        1003
#define IDC_EVENTSEARCH_GENRE                1004
#define IDC_EVENTSEARCH_GENRE_CHECKALL       1005
#define IDC_EVENTSEARCH_GENRE_UNCHECKALL     1006
#define IDC_EVENTSEARCH_GENRE_STATUS         1007
#define IDC_EVENTSEARCH_DAYOFWEEK            1008
#define IDC_EVENTSEARCH_DAYOFWEEK_SUNDAY     1009
#define IDC_EVENTSEARCH_DAYOFWEEK_MONDAY     1010
#define IDC_EVENTSEARCH_DAYOFWEEK_TUESDAY    1011
#define IDC_EVENTSEARCH_DAYOFWEEK_WEDNESDAY  1012
#define IDC_EVENTSEARCH_DAYOFWEEK_THURSDAY   1013
#define IDC_EVENTSEARCH_DAYOFWEEK_FRIDAY     1014
#define IDC_EVENTSEARCH_DAYOFWEEK_SATURDAY   1015
#define IDC_EVENTSEARCH_TIME                 1016
#define IDC_EVENTSEARCH_TIME_START_HOUR      1017
#define IDC_EVENTSEARCH_TIME_START_SEPARATOR 1018
#define IDC_EVENTSEARCH_TIME_START_MINUTE    1019
#define IDC_EVENTSEARCH_TIME_RANGE           1020
#define IDC_EVENTSEARCH_TIME_END_HOUR        1021
#define IDC_EVENTSEARCH_TIME_END_SEPARATOR   1022
#define IDC_EVENTSEARCH_TIME_END_MINUTE      1023
#define IDC_EVENTSEARCH_DURATION             1024
#define IDC_EVENTSEARCH_DURATION_SHORT_INPUT 1025
#define IDC_EVENTSEARCH_DURATION_SHORT_SPIN  1026
#define IDC_EVENTSEARCH_DURATION_SHORT_UNIT  1027
#define IDC_EVENTSEARCH_CA                   1028
#define IDC_EVENTSEARCH_CA_LIST              1029
#define IDC_EVENTSEARCH_VIDEO                1030
#define IDC_EVENTSEARCH_VIDEO_LIST           1031
#define IDC_EVENTSEARCH_HIGHLIGHT            1032
#define IDC_EVENTSEARCH_SETTINGSLIST         1033
#define IDC_EVENTSEARCH_SETTINGSLIST_SAVE    1034
#define IDC_EVENTSEARCH_SETTINGSLIST_DELETE  1035
#define IDC_EVENTSEARCH_SEARCHTARGET         1036
#define IDC_EVENTSEARCH_KEYWORDMENU          1040
#define IDC_EVENTSEARCH_DELETEKEYWORD        1041
#define IDC_EVENTSEARCH_CLEARKEYWORDHISTORY  1042
#define IDC_EVENTSEARCH_SEARCH               1050

#define IDC_PROGRAMSEARCH_SETTINGSPLACE 1000
#define IDC_PROGRAMSEARCH_STATUS        1036
#define IDC_PROGRAMSEARCH_RESULT        1037
#define IDC_PROGRAMSEARCH_INFO          1038
#define IDC_PROGRAMSEARCH_RESULTPANE    1039

#define IDC_EPGCHANNELSETTINGS_CHANNELLIST    1000
#define IDC_EPGCHANNELSETTINGS_CHECKALL       1001
#define IDC_EPGCHANNELSETTINGS_UNCHECKALL     1002
#define IDC_EPGCHANNELSETTINGS_INVERTCHECK    1003
#define IDC_EPGCHANNELSETTINGS_EXCLUDENOEVENT 1004

#define IDC_STREAMPROPERTIES_TAB 1000

#define IDC_STREAMINFO_STREAM  1000
#define IDC_STREAMINFO_NETWORK 1001
#define IDC_STREAMINFO_SERVICE 1002
#define IDC_STREAMINFO_UPDATE  1003
#define IDC_STREAMINFO_COPY    1004

#define IDC_PIDINFO_LIST 1000
#define IDC_PIDINFO_COPY 1001

#define IDC_TSPROCESSORERROR_ICON       1000
#define IDC_TSPROCESSORERROR_MESSAGE    1001
#define IDC_TSPROCESSORERROR_RETRY      1002
#define IDC_TSPROCESSORERROR_NOFILTER   1003
#define IDC_TSPROCESSORERROR_DEVICELIST 1004
#define IDC_TSPROCESSORERROR_FILTERLIST 1005
#define IDC_TSPROCESSORERROR_SEARCH     1006

#define IDC_ZOOMOPTIONS_LIST         1000
#define IDC_ZOOMOPTIONS_TYPE_RATE    1001
#define IDC_ZOOMOPTIONS_TYPE_SIZE    1002
#define IDC_ZOOMOPTIONS_RATE_LABEL   1003
#define IDC_ZOOMOPTIONS_RATE         1004
#define IDC_ZOOMOPTIONS_RATE_UNIT    1005
#define IDC_ZOOMOPTIONS_WIDTH_LABEL  1006
#define IDC_ZOOMOPTIONS_WIDTH        1007
#define IDC_ZOOMOPTIONS_HEIGHT_LABEL 1008
#define IDC_ZOOMOPTIONS_HEIGHT       1009
#define IDC_ZOOMOPTIONS_GETCURSIZE   1010
#define IDC_ZOOMOPTIONS_UP           1011
#define IDC_ZOOMOPTIONS_DOWN         1012

#define IDC_PANANDSCAN_LIST        1000
#define IDC_PANANDSCAN_UP          1001
#define IDC_PANANDSCAN_DOWN        1002
#define IDC_PANANDSCAN_REMOVE      1003
#define IDC_PANANDSCAN_CLEAR       1004
#define IDC_PANANDSCAN_IMPORT      1005
#define IDC_PANANDSCAN_EXPORT      1006
#define IDC_PANANDSCAN_ADD         1007
#define IDC_PANANDSCAN_REPLACE     1008
#define IDC_PANANDSCAN_NAME        1009
#define IDC_PANANDSCAN_XPOS        1010
#define IDC_PANANDSCAN_XPOS_SPIN   1011
#define IDC_PANANDSCAN_YPOS        1012
#define IDC_PANANDSCAN_YPOS_SPIN   1013
#define IDC_PANANDSCAN_WIDTH       1014
#define IDC_PANANDSCAN_WIDTH_SPIN  1015
#define IDC_PANANDSCAN_HEIGHT      1016
#define IDC_PANANDSCAN_HEIGHT_SPIN 1017
#define IDC_PANANDSCAN_XASPECT     1018
#define IDC_PANANDSCAN_YASPECT     1019
#define IDC_PANANDSCAN_PREVIEW     1020
#define IDC_PANANDSCAN_TEST        1021

#define IDC_FAVORITES_FOLDERTREE 1000
#define IDC_FAVORITES_DELETE     1010
#define IDC_FAVORITES_RENAME     1011
#define IDC_FAVORITES_PROPERTIES 1012
#define IDC_FAVORITES_NEWFOLDER  1020

#define IDC_FAVORITEPROP_NAME                 1000
#define IDC_FAVORITEPROP_BONDRIVER            1001
#define IDC_FAVORITEPROP_FORCEBONDRIVERCHANGE 1002
#define IDC_FAVORITEPROP_SERVICEID            1003
#define IDC_FAVORITEPROP_NETWORKID            1004
#define IDC_FAVORITEPROP_TRANSPORTSTREAMID    1005

#define IDC_PROGRAMGUIDEFAVORITES_LIST           1000
#define IDC_PROGRAMGUIDEFAVORITES_NAME_LABEL     1001
#define IDC_PROGRAMGUIDEFAVORITES_NAME           1002
#define IDC_PROGRAMGUIDEFAVORITES_COLORS_LABEL   1003
#define IDC_PROGRAMGUIDEFAVORITES_COLORS_PREVIEW 1004
#define IDC_PROGRAMGUIDEFAVORITES_BACKCOLOR      1005
#define IDC_PROGRAMGUIDEFAVORITES_TEXTCOLOR      1006
#define IDC_PROGRAMGUIDEFAVORITES_UP             1010
#define IDC_PROGRAMGUIDEFAVORITES_DOWN           1011
#define IDC_PROGRAMGUIDEFAVORITES_DELETE         1012
#define IDC_PROGRAMGUIDEFAVORITES_FIXEDWIDTH     1013

#define IDC_PROGRAMGUIDETOOLBAR_ITEMLIST               1000
#define IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_UP            1001
#define IDC_PROGRAMGUIDETOOLBAR_ITEMLIST_DOWN          1002
#define IDC_PROGRAMGUIDETOOLBAR_DATEBAR_BUTTONCOUNT    1010
#define IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_INTERVAL  1020
#define IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_TIME_CUSTOM    1021
#define IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL       1022
#define IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_INTERVAL_UNIT  1023
#define IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_CUSTOMTIME     1024
#define IDC_PROGRAMGUIDETOOLBAR_TIMEBAR_MAXBUTTONCOUNT 1025

#define IDC_FEATUREDEVENTS_SERVICELIST                  1000
#define IDC_FEATUREDEVENTS_SERVICELIST_CHECKALL         1001
#define IDC_FEATUREDEVENTS_SERVICELIST_UNCHECKALL       1002
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST           1010
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_EDIT      1011
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DELETE    1012
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_ADD       1013
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DUPLICATE 1014
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_UP        1015
#define IDC_FEATUREDEVENTS_SEARCHSETTINGSLIST_DOWN      1016
#define IDC_FEATUREDEVENTS_PERIOD_LABEL                 1020
#define IDC_FEATUREDEVENTS_PERIOD                       1021
#define IDC_FEATUREDEVENTS_PERIOD_SPIN                  1022
#define IDC_FEATUREDEVENTS_PERIOD_UNIT                  1023
#define IDC_FEATUREDEVENTS_EVENTTEXTLINES_LABEL         1024
#define IDC_FEATUREDEVENTS_EVENTTEXTLINES               1025
#define IDC_FEATUREDEVENTS_EVENTTEXTLINES_SPIN          1026
#define IDC_FEATUREDEVENTS_EVENTTEXTLINES_UNIT          1027

#define IDC_FEATUREDEVENTSSEARCH_NAME                   1000
#define IDC_FEATUREDEVENTSSEARCH_SETTINGSPLACE          1001
#define IDC_FEATUREDEVENTSSEARCH_USESERVICELIST         1002
#define IDC_FEATUREDEVENTSSEARCH_SERVICELIST            1003
#define IDC_FEATUREDEVENTSSEARCH_SERVICELIST_CHECKALL   1004
#define IDC_FEATUREDEVENTSSEARCH_SERVICELIST_UNCHECKALL 1005

#define IDC_PROPERTYPAGEFRAME_TAB   1000
#define IDC_PROPERTYPAGEFRAME_APPLY 1001
#define IDC_PROPERTYPAGEFRAME_HELP  1002

#define IDC_SURROUND_USECUSTOMDOWNMIXMATRIX     1000
#define IDC_SURROUND_DOWNMIXMATRIX_FL_LABEL     1001
#define IDC_SURROUND_DOWNMIXMATRIX_FR_LABEL     1002
#define IDC_SURROUND_DOWNMIXMATRIX_FC_LABEL     1003
#define IDC_SURROUND_DOWNMIXMATRIX_LFE_LABEL    1004
#define IDC_SURROUND_DOWNMIXMATRIX_SL_LABEL     1005
#define IDC_SURROUND_DOWNMIXMATRIX_SR_LABEL     1006
#define IDC_SURROUND_DOWNMIXMATRIX_L_LABEL      1007
#define IDC_SURROUND_DOWNMIXMATRIX_R_LABEL      1008
#define IDC_SURROUND_DOWNMIXMATRIX_L_FL         1009
#define IDC_SURROUND_DOWNMIXMATRIX_L_FR         1010
#define IDC_SURROUND_DOWNMIXMATRIX_L_FC         1011
#define IDC_SURROUND_DOWNMIXMATRIX_L_LFE        1012
#define IDC_SURROUND_DOWNMIXMATRIX_L_SL         1013
#define IDC_SURROUND_DOWNMIXMATRIX_L_SR         1014
#define IDC_SURROUND_DOWNMIXMATRIX_R_FL         1015
#define IDC_SURROUND_DOWNMIXMATRIX_R_FR         1016
#define IDC_SURROUND_DOWNMIXMATRIX_R_FC         1017
#define IDC_SURROUND_DOWNMIXMATRIX_R_LFE        1018
#define IDC_SURROUND_DOWNMIXMATRIX_R_SL         1019
#define IDC_SURROUND_DOWNMIXMATRIX_R_SR         1020
#define IDC_SURROUND_DOWNMIXMATRIX_DEFAULT      1021
#define IDC_SURROUND_USECUSTOMMIXINGMATRIX      1022
#define IDC_SURROUND_MIXINGMATRIX_IN_FL_LABEL   1023
#define IDC_SURROUND_MIXINGMATRIX_IN_FR_LABEL   1024
#define IDC_SURROUND_MIXINGMATRIX_IN_FC_LABEL   1025
#define IDC_SURROUND_MIXINGMATRIX_IN_LFE_LABEL  1026
#define IDC_SURROUND_MIXINGMATRIX_IN_SL_LABEL   1027
#define IDC_SURROUND_MIXINGMATRIX_IN_SR_LABEL   1028
#define IDC_SURROUND_MIXINGMATRIX_OUT_FL_LABEL  1029
#define IDC_SURROUND_MIXINGMATRIX_OUT_FR_LABEL  1030
#define IDC_SURROUND_MIXINGMATRIX_OUT_FC_LABEL  1031
#define IDC_SURROUND_MIXINGMATRIX_OUT_LFE_LABEL 1032
#define IDC_SURROUND_MIXINGMATRIX_OUT_SL_LABEL  1033
#define IDC_SURROUND_MIXINGMATRIX_OUT_SR_LABEL  1034
#define IDC_SURROUND_MIXINGMATRIX_FL_FL         1035
#define IDC_SURROUND_MIXINGMATRIX_FL_FR         1036
#define IDC_SURROUND_MIXINGMATRIX_FL_FC         1037
#define IDC_SURROUND_MIXINGMATRIX_FL_LFE        1038
#define IDC_SURROUND_MIXINGMATRIX_FL_SL         1039
#define IDC_SURROUND_MIXINGMATRIX_FL_SR         1040
#define IDC_SURROUND_MIXINGMATRIX_FR_FL         1041
#define IDC_SURROUND_MIXINGMATRIX_FR_FR         1042
#define IDC_SURROUND_MIXINGMATRIX_FR_FC         1043
#define IDC_SURROUND_MIXINGMATRIX_FR_LFE        1044
#define IDC_SURROUND_MIXINGMATRIX_FR_SL         1045
#define IDC_SURROUND_MIXINGMATRIX_FR_SR         1046
#define IDC_SURROUND_MIXINGMATRIX_FC_FL         1047
#define IDC_SURROUND_MIXINGMATRIX_FC_FR         1048
#define IDC_SURROUND_MIXINGMATRIX_FC_FC         1049
#define IDC_SURROUND_MIXINGMATRIX_FC_LFE        1050
#define IDC_SURROUND_MIXINGMATRIX_FC_SL         1051
#define IDC_SURROUND_MIXINGMATRIX_FC_SR         1052
#define IDC_SURROUND_MIXINGMATRIX_LFE_FL        1053
#define IDC_SURROUND_MIXINGMATRIX_LFE_FR        1054
#define IDC_SURROUND_MIXINGMATRIX_LFE_FC        1055
#define IDC_SURROUND_MIXINGMATRIX_LFE_LFE       1056
#define IDC_SURROUND_MIXINGMATRIX_LFE_SL        1057
#define IDC_SURROUND_MIXINGMATRIX_LFE_SR        1058
#define IDC_SURROUND_MIXINGMATRIX_SL_FL         1059
#define IDC_SURROUND_MIXINGMATRIX_SL_FR         1060
#define IDC_SURROUND_MIXINGMATRIX_SL_FC         1061
#define IDC_SURROUND_MIXINGMATRIX_SL_LFE        1062
#define IDC_SURROUND_MIXINGMATRIX_SL_SL         1063
#define IDC_SURROUND_MIXINGMATRIX_SL_SR         1064
#define IDC_SURROUND_MIXINGMATRIX_SR_FL         1065
#define IDC_SURROUND_MIXINGMATRIX_SR_FR         1066
#define IDC_SURROUND_MIXINGMATRIX_SR_FC         1067
#define IDC_SURROUND_MIXINGMATRIX_SR_LFE        1068
#define IDC_SURROUND_MIXINGMATRIX_SR_SL         1069
#define IDC_SURROUND_MIXINGMATRIX_SR_SR         1070

#define IDC_CHANNELINPUT_DIGITKEYMODE    1000
#define IDC_CHANNELINPUT_NUMPADKEYMODE   1001
#define IDC_CHANNELINPUT_FUNCTIONKEYMODE 1002
#define IDC_CHANNELINPUT_TIMEOUT         1003
#define IDC_CHANNELINPUT_TIMEOUTMODE     1004
#define IDC_CHANNELINPUT_HELP            1005

#define IDC_DIRECTWRITEOPTIONS_RENDERINGMODE_ENABLE    1000
#define IDC_DIRECTWRITEOPTIONS_RENDERINGMODE           1001
#define IDC_DIRECTWRITEOPTIONS_GAMMA_ENABLE            1002
#define IDC_DIRECTWRITEOPTIONS_GAMMA                   1003
#define IDC_DIRECTWRITEOPTIONS_GAMMA_RANGE             1004
#define IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_ENABLE 1005
#define IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST        1006
#define IDC_DIRECTWRITEOPTIONS_ENHANCEDCONTRAST_RANGE  1007
#define IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_ENABLE   1008
#define IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL          1009
#define IDC_DIRECTWRITEOPTIONS_CLEARTYPELEVEL_RANGE    1010
#define IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY_ENABLE    1011
#define IDC_DIRECTWRITEOPTIONS_PIXELGEOMETRY           1012
#define IDC_DIRECTWRITEOPTIONS_PREVIEW                 1013
#define IDC_DIRECTWRITEOPTIONS_TEST                    1014
