#if defined(HAVE_OPENCL)

#include "DeviceArray.h"

DeviceArray::DeviceArray()
	: _width(0)
	, _height(0)
	, _size(0)
{
}

DeviceArray DeviceArray::create(clw::Context& context,
								clw::EAccess access, 
								clw::EMemoryLocation location,
								int width, 
								int height, 
								EDataType dataType)
{
	size_t size = width * height * dataSize(dataType);
	clw::Buffer buffer = context.createBuffer(access, location, size);
	DeviceArray deviceArray;
	if(!buffer.isNull())
	{
		deviceArray._buffer = std::move(buffer);
		deviceArray._width = width;
		deviceArray._height = height;
		deviceArray._dataType = dataType;
		deviceArray._size = size;
	}

	return deviceArray;
}

void DeviceArray::truncate(int height)
{
	if(_height > height)
	{
		_height = height;
		_size = _width * _height * dataSize(_dataType);
	}
}

clw::Event DeviceArray::upload(clw::CommandQueue& queue, const void* data)
{
	/// TODO Benchmark direct access with mapped one
	/// TODO Check pinned memory
	if(_buffer.isNull())
		return clw::Event();
	return queue.asyncWriteBuffer(_buffer, data, 0, _size);
}

clw::Event DeviceArray::download(clw::CommandQueue& queue, void* data) const
{
	if(_buffer.isNull())
		return clw::Event();
	return queue.asyncReadBuffer(_buffer, data, 0, _size);
}

clw::Event DeviceArray::download(clw::CommandQueue& queue, cv::Mat& mat) const
{
	if(_buffer.isNull())
		return clw::Event();

	mat.create(_height, _width, deviceToMatType(_dataType));
	return queue.asyncReadBuffer(_buffer, mat.data, 0, _size);
}

#endif