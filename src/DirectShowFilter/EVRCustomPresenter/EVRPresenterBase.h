#pragma once


#include <cmath>
#include <new>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <evr9.h>
#include <evcode.h>

#include "EVRHelpers.h"
#include "EVRMediaType.h"

#pragma comment(lib, "evr_vista.lib")
#pragma comment(lib, "mf_vista.lib")
#pragma comment(lib, "mfplat_vista.lib")
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dxva2.lib")


// {CB9FCC04-4247-47A8-9B55-4CCB1D2CC95B}
static const GUID SampleAttribute_Counter = {
	0xCB9FCC04, 0x4247, 0x47A8, {0x9B, 0x55, 0x4C, 0xCB, 0x1D, 0x2C, 0xC9, 0x5B}
};

// {99F1B32D-B439-424A-B2E9-8877C6E0DDFD}
static const GUID SampleAttribute_SwapChain = {
	0x99F1B32D, 0xB439, 0x424A, {0xB2, 0xE9, 0x88, 0x77, 0xC6, 0xE0, 0xDD, 0xFD}
};
