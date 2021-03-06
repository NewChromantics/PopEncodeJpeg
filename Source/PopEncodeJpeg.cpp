#include "PopEncodeJpeg.hpp"
#include <exception>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <algorithm>
#include "TStringBuffer.hpp"

#define ENABLE_FOPEN	0
#define TJE_IMPLEMENTATION
#include "TinyJpeg/tiny_jpeg.h"




#if defined(TARGET_WINDOWS)
BOOL APIENTRY DllMain(HMODULE /* hModule */, DWORD ul_reason_for_call, LPVOID /* lpReserved */)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
#endif


class TWriteContext
{
public:
	TWriteContext(uint8_t* JpegData,size_t JpegDataSize) :
		mJpegData		( JpegData ),
		mJpegDataSize	( JpegDataSize ),
		mDataWritten	( 0 )
	{
	}
	
	void		Write(const uint8_t* Data,size_t Size);

public:
	uint8_t*	mJpegData;
	size_t		mJpegDataSize;
	size_t		mDataWritten;
};


namespace PopEncodeJpeg
{
	std::shared_ptr<TStringBuffer>	gDebugStrings;
	TStringBuffer&					GetDebugStrings();
}


template<typename STRING>
void DebugLog(const STRING& String)
{
	auto& DebugStrings = PopEncodeJpeg::GetDebugStrings();
	DebugStrings.Push( String );
}


__export const char* PopDebugString()
{
	try
	{
		auto& DebugStrings = PopEncodeJpeg::GetDebugStrings();
		return DebugStrings.Pop();
	}
	catch(...)
	{
		//	bit recursive if we push one?
		return nullptr;
	}
}

__export void ReleaseDebugString(const char* String)
{
	try
	{
		auto& DebugStrings = PopEncodeJpeg::GetDebugStrings();
		DebugStrings.Release( String );
	}
	catch(...)
	{
	}
}



__export int32_t	EncodeJpeg(uint8_t* JpegData,int32_t JpegDataSize,int32_t JpegQuality,uint8_t* ImageData,int32_t ImageDataSize,int32_t ImageWidth,int32_t ImageHeight,int32_t ImageComponents,int32_t IsRgb)
{
	//	non-capturing lambda
	auto WriteToContext = [](void* context, void* data, int size)
	{
		if ( size < 0 )
		{
			std::stringstream Error;
			Error << "Trying to write negative size " << size;
			throw std::range_error::range_error( Error.str() );
		}
		auto& Context = *reinterpret_cast<TWriteContext*>( context );
		Context.Write( reinterpret_cast<const uint8_t*>(data), size );
	};

	try
	{
		TWriteContext Context( JpegData, JpegDataSize );
		
		bool DataIsRgb = (IsRgb!=0);
		auto Result = tje_encode_with_func( WriteToContext, &Context, JpegQuality, ImageWidth, ImageHeight, ImageComponents, DataIsRgb, ImageData );
		if ( Result != 1 )
		{
			std::stringstream Error;
			Error << "Error encoding: " << Result;
			throw std::runtime_error::runtime_error( Error.str() );
		}
		auto Written = static_cast<int32_t>( Context.mDataWritten );
		return Written;
	}
	catch(std::exception& e)
	{
		DebugLog( e.what() );
		return 0;
	}
}


TStringBuffer& PopEncodeJpeg::GetDebugStrings()
{
	if ( !gDebugStrings )
	{
		gDebugStrings.reset( new TStringBuffer() );
	}
	return *gDebugStrings;
}



void TWriteContext::Write(const uint8_t* Data,size_t Size)
{
	//	gr: keep going so we know how much we need
	/*
	if ( mDataWritten + Size > mJpegDataSize )
	{
		std::stringstream Error;
		Error << "Jpeg buffer size not big enough, trying to write " << (mDataWritten + Size) << "/" << mJpegDataSize;
		throw std::runtime_error::runtime_error( Error.str() );
	}
	*/
	
	auto SpaceRemaining = mJpegDataSize - mDataWritten;
	auto WriteLength = std::min( SpaceRemaining, Size );
	if ( WriteLength > 0 )
	{
		auto* Dst = &mJpegData[mDataWritten];
		auto* Src = Data;
		memcpy( Dst, Src, WriteLength );
	}
	
	mDataWritten += Size;
}


