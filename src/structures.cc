#include <node_buffer.h>
#include <nan.h>

extern "C" {
#include <string.h>
}

#include "structures.h"
#include "common.h"

using namespace v8;
using namespace node;

Local<Object> js_OCClientResponse( OCClientResponse *response ) {
	uint32_t optionIndex, optionDataIndex;
	Local<Object> jsResponse = NanNew<Object>();

		// jsResponse.addr
		Local<Object> addr = NanNew<Object>();

			// jsResponse.addr.size
			addr->Set( NanNew<String>( "size" ), NanNew<Number>( response->addr->size ) );

			// jsResponse.addr.addr
			Local<Array> addrAddr = NanNew<Array>( response->addr->size );
			for ( optionIndex = 0 ; optionIndex < response->addr->size ; optionIndex++ ) {
				addrAddr->Set( optionIndex, NanNew<Number>(
					response->addr->addr[ optionIndex ] ) );
			}
			addr->Set( NanNew<String>( "addr" ), addrAddr );

		jsResponse->Set( NanNew<String>( "addr" ), addr );

		// jsResponse.connType
		jsResponse->Set( NanNew<String>( "connType" ), NanNew<Number>( response->connType ) );

		// jsResponse.result
		jsResponse->Set( NanNew<String>( "result" ), NanNew<Number>( response->result ) );

		// jsResponse.sequenceNumber
		jsResponse->Set( NanNew<String>( "sequenceNumber" ),
			NanNew<Number>( response->sequenceNumber ) );

		// jsResponse.resJSONPayload
		if ( response->resJSONPayload ) {
			jsResponse->Set( NanNew<String>( "resJSONPayload" ),
				NanNew<String>( response->resJSONPayload ) );
		}

		// jsResponse.numRcvdVendorSpecificHeaderOptions
		jsResponse->Set(
			NanNew<String>( "numRcvdVendorSpecificHeaderOptions" ),
			NanNew<Number>( response->numRcvdVendorSpecificHeaderOptions ) );

		// jsResponse.rcvdVendorSpecificHeaderOptions
		Local<Array> headerOptions = NanNew<Array>( response->numRcvdVendorSpecificHeaderOptions );

			// jsResponse.rcvdVendorSpecificHeaderOptions[ index ]
			OCHeaderOption *cHeaderOptions = response->rcvdVendorSpecificHeaderOptions;
			uint8_t headerOptionCount = response->numRcvdVendorSpecificHeaderOptions;
			for ( optionIndex = 0 ;
					optionIndex < headerOptionCount ;
					optionIndex++ ) {
				Local<Object> headerOption = NanNew<Object>();

					// response.rcvdVendorSpecificHeaderOptions[ index ].protocolID
					headerOption->Set( NanNew<String>( "protocolID" ),
						NanNew<Number>( cHeaderOptions[ optionIndex ].protocolID ) );

					// response.rcvdVendorSpecificHeaderOptions[ index ].optionID
					headerOption->Set( NanNew<String>( "optionID" ),
						NanNew<Number>( cHeaderOptions[ optionIndex ].optionID ) );

					// response.rcvdVendorSpecificHeaderOptions[ index ].optionLength
					headerOption->Set( NanNew<String>( "optionLength" ),
						NanNew<Number>( cHeaderOptions[ optionIndex ].optionLength ) );

					// response.rcvdVendorSpecificHeaderOptions[ index ].optionData
					Local<Array> headerOptionData = NanNew<Array>(
						cHeaderOptions[ optionIndex ].optionLength );
					for ( optionDataIndex = 0 ;
							optionDataIndex < cHeaderOptions[ optionIndex ].optionLength ;
							optionDataIndex++ ) {
						headerOptionData->Set(
							optionDataIndex,
							NanNew<Number>(
								cHeaderOptions[ optionIndex ].optionData[ optionDataIndex ] ) );
					}
					headerOption->Set( NanNew<String>( "optionData" ), headerOptionData );
				headerOptions->Set( optionIndex, headerOption );
			}
		jsResponse->Set( NanNew<String>( "rcvdVendorSpecificHeaderOptions" ), headerOptions );

	return jsResponse;
}

bool c_OCEntityHandlerResponse(
		OCEntityHandlerResponse *destination,
		v8::Local<Object> jsOCEntityHandlerResponse ) {

	// requestHandle
	Local<Value> requestHandle =
		jsOCEntityHandlerResponse->Get( NanNew<String>( "requestHandle" ) );
	if ( !Buffer::HasInstance( requestHandle ) ) {
		NanThrowTypeError( "requestHandle is not a Node::Buffer" );
		return false;
	}
	destination->requestHandle = *( OCRequestHandle * )Buffer::Data( requestHandle->ToObject() );

	// responseHandle is filled in by the stack

	// resourceHandle
	Local<Value> resourceHandle =
		jsOCEntityHandlerResponse->Get( NanNew<String>( "resourceHandle" ) );
	if ( !Buffer::HasInstance( resourceHandle ) ) {
		NanThrowTypeError( "resourceHandle is not a Node::Buffer" );
		return false;
	}
	destination->resourceHandle = *( OCResourceHandle * )Buffer::Data( resourceHandle->ToObject() );

	// ehResult
	Local<Value> ehResult = jsOCEntityHandlerResponse->Get( NanNew<String>( "ehResult" ) );
	VALIDATE_VALUE_TYPE( ehResult, IsUint32, "ehResult", false );
	destination->ehResult = ( OCEntityHandlerResult )ehResult->Uint32Value();

	// payload
	Local<Value> payload = jsOCEntityHandlerResponse->Get( NanNew<String>( "payload" ) );
	VALIDATE_VALUE_TYPE( payload, IsString, "payload", false );
	const char *payloadString = ( const char * )*String::Utf8Value( payload );
	size_t payloadLength = strlen( payloadString );
	if ( payloadLength >= MAX_RESPONSE_LENGTH ) {
		NanThrowRangeError( "payload is longer than MAX_RESPONSE_LENGTH" );
		return false;
	}
	strncpy( destination->payload, payloadString, payloadLength );

	// payloadSize
	destination->payloadSize = payloadLength;

	// numSendVendorSpecificHeaderOptions
	Local<Value> numSendVendorSpecificHeaderOptions = jsOCEntityHandlerResponse->Get(
		NanNew<String>( "numSendVendorSpecificHeaderOptions" ) );
	VALIDATE_VALUE_TYPE(
		numSendVendorSpecificHeaderOptions,
		IsUint32,
		"numSendVendorSpecificHeaderOptions",
		false );
	uint8_t headerOptionCount = ( uint8_t )numSendVendorSpecificHeaderOptions->Uint32Value();
	if ( headerOptionCount > MAX_HEADER_OPTIONS ) {
		NanThrowRangeError(
			"numSendVendorSpecificHeaderOptions is larger than MAX_HEADER_OPTIONS" );
		return false;
	}
	destination->numSendVendorSpecificHeaderOptions = headerOptionCount;

	// sendVendorSpecificHeaderOptions
	int headerOptionIndex, optionDataIndex;
	Local<Value> headerOptionsValue = jsOCEntityHandlerResponse->Get(
		NanNew<String>( "sendVendorSpecificHeaderOptions" ) );
	VALIDATE_VALUE_TYPE(
		headerOptionsValue,
		IsArray,
		"sendVendorSpecificHeaderOptions",
		false );
	Local<Array> headerOptions = Local<Array>::Cast( headerOptionsValue );
	for ( headerOptionIndex = 0 ; headerOptionIndex < headerOptionCount; headerOptionIndex++ ) {
		Local<Value> headerOptionValue = headerOptions->Get( headerOptionIndex );
		VALIDATE_VALUE_TYPE(
			headerOptionValue,
			IsObject,
			"sendVendorSpecificHeaderOptions member",
			false );
		Local<Object> headerOption = headerOptionValue->ToObject();

		// sendVendorSpecificHeaderOptions[].protocolID
		Local<Value> protocolIDValue = headerOption->Get( NanNew<String>( "protocolID" ) );
		VALIDATE_VALUE_TYPE( protocolIDValue, IsUint32, "protocolID", false );
		destination->sendVendorSpecificHeaderOptions[ headerOptionIndex ].protocolID =
			( OCTransportProtocolID )protocolIDValue->Uint32Value();

		// sendVendorSpecificHeaderOptions[].optionID
		Local<Value> optionIDValue = headerOption->Get(
			NanNew<String>( "optionID" ) );
		VALIDATE_VALUE_TYPE( optionIDValue, IsUint32, "optionID", false );
		destination->sendVendorSpecificHeaderOptions[ headerOptionIndex ].optionID =
			( uint16_t )protocolIDValue->Uint32Value();

		// sendVendorSpecificHeaderOptions[].optionLength
		Local<Value> optionLengthValue = headerOption->Get(
			NanNew<String>( "optionLength" ) );
		VALIDATE_VALUE_TYPE( optionLengthValue, IsUint32, "optionLength", false );
		uint16_t optionLength = ( uint16_t )optionLengthValue->Uint32Value();
		if ( optionLength > MAX_HEADER_OPTION_DATA_LENGTH ) {
			NanThrowRangeError( "optionLength is larger than MAX_HEADER_OPTION_DATA_LENGTH" );
			return false;
		}
		destination->sendVendorSpecificHeaderOptions[ headerOptionIndex ].optionLength =
			optionLength;

		// sendVendorSpecificHeaderOptions[].optionData
		Local<Value> optionDataValue = headerOption->Get( NanNew<String>( "optionData" ) );
		VALIDATE_VALUE_TYPE( optionDataValue, IsArray, "optionData", false );
		Local<Array> optionData = Local<Array>::Cast( optionDataValue );
		for ( optionDataIndex = 0 ; optionDataIndex < optionLength ; optionDataIndex++ ) {
			Local<Value> optionDataItemValue = optionData->Get( optionDataIndex );
			VALIDATE_VALUE_TYPE( optionDataItemValue, IsUint32, "optionData item", false );
			destination->sendVendorSpecificHeaderOptions[ headerOptionIndex ]
				.optionData[ optionDataIndex ] = ( uint8_t )optionDataItemValue->Uint32Value();
		}
	}

	return true;
}