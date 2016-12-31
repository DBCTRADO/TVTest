#define IDI_MAIN            1
#define IDB_ICON            2
#define IDB_CAPTURE         3
#define IDB_CAPTURE_ADD     4
#define IDB_TOOLBAR_16      5
#define IDB_TOOLBAR_32      6

#define IDM_CONTEXT_MENU    10

#define IDD_SETTINGS        20

#define CM_CAPTURE                  100
#define CM_CAPTURE_ADD              101
#define CM_SAVE                     102
#define CM_SAVE_AS                  103
#define CM_SAVE_ALL                 104
#define CM_COPY                     105
#define CM_PREV_FRAME               106
#define CM_NEXT_FRAME               107
#define CM_FIRST_FRAME              108
#define CM_LAST_FRAME               109
#define CM_SKIP_BACKWARD_FRAME      110
#define CM_SKIP_FORWARD_FRAME       111
#define CM_CAPTURE_SIZE_FIRST       120
#define CM_CAPTURE_SIZE_LAST        219
#define CM_RESAMPLE_FIRST           220
#define CM_RESAMPLE_LAST            (CM_RESAMPLE_FIRST + 4)
#define CM_RESAMPLE_NEAREST         (CM_RESAMPLE_FIRST + 0)
#define CM_RESAMPLE_BILINEAR        (CM_RESAMPLE_FIRST + 1)
#define CM_RESAMPLE_AVERAGING       (CM_RESAMPLE_FIRST + 2)
#define CM_RESAMPLE_LANCZOS2        (CM_RESAMPLE_FIRST + 3)
#define CM_RESAMPLE_LANCZOS3        (CM_RESAMPLE_FIRST + 4)
#define CM_DEINTERLACE_FIRST        230
#define CM_DEINTERLACE_LAST         (CM_DEINTERLACE_FIRST + 5)
#define CM_DEINTERLACE_WEAVE        (CM_DEINTERLACE_FIRST + 0)
#define CM_DEINTERLACE_BLEND        (CM_DEINTERLACE_FIRST + 1)
#define CM_DEINTERLACE_BOB          (CM_DEINTERLACE_FIRST + 2)
#define CM_DEINTERLACE_ELA          (CM_DEINTERLACE_FIRST + 3)
#define CM_DEINTERLACE_YADIF        (CM_DEINTERLACE_FIRST + 4)
#define CM_DEINTERLACE_YADIF_BOB    (CM_DEINTERLACE_FIRST + 5)
#define CM_FIT_IMAGE_TO_WINDOW      240
#define CM_ZOOM_FIRST               241
#define CM_ZOOM_LAST                (CM_ZOOM_FIRST + 7)
#define CM_ZOOM_25                  (CM_ZOOM_FIRST + 0)
#define CM_ZOOM_33                  (CM_ZOOM_FIRST + 1)
#define CM_ZOOM_50                  (CM_ZOOM_FIRST + 2)
#define CM_ZOOM_66                  (CM_ZOOM_FIRST + 3)
#define CM_ZOOM_75                  (CM_ZOOM_FIRST + 4)
#define CM_ZOOM_100                 (CM_ZOOM_FIRST + 5)
#define CM_ZOOM_150                 (CM_ZOOM_FIRST + 6)
#define CM_ZOOM_200                 (CM_ZOOM_FIRST + 7)
#define CM_FIT_WINDOW_TO_IMAGE      250
#define CM_SETTINGS                 251

#define IDC_SETTINGS_MEMORY_SIZE            1000
#define IDC_SETTINGS_ESTIMATE_MEMORY_USAGE  1001
#define IDC_SETTINGS_ACCUMULATE_ALWAYS      1002
#define IDC_SETTINGS_SKIP_FRAMES            1003
