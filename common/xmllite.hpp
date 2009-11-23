/*
	Copyright (C) 2007 Cory Nelson

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <stdexcept>

#include <windows.h>
#include <atlbase.h>
#include <xmllite.h>

/*
	xml_error is thrown from xml_reader or xml_writer.
*/
class xml_error : public std::runtime_error {
public:
	xml_error(const char *msg, HRESULT res) : std::runtime_error(msg),m_res(res) {}

	// gets the actual HRESULT error code.
	HRESULT result() const { return m_res; }
	
private:
	const HRESULT m_res;
};

/*
	xml_reader is a light wrapper for xmllite's IXmlReader.
*/
class xml_reader {
public:
	enum node_type {
		none = XmlNodeType_None,
		element = XmlNodeType_Element,
		attribute = XmlNodeType_Attribute,
		text = XmlNodeType_Text,
		cdata = XmlNodeType_CDATA,
		procinstruction = XmlNodeType_ProcessingInstruction,
		comment = XmlNodeType_Comment,
		doctype = XmlNodeType_DocumentType,
		whitespace = XmlNodeType_Whitespace,
		endelement = XmlNodeType_EndElement,
		xmldecl = XmlNodeType_XmlDeclaration
	};

	// constructs a closed reader.
	xml_reader() {}

	// constructs a reader and opens a file.
	xml_reader(LPCTSTR file) { open(file); }

	~xml_reader() { close(); }

	// opens the reader.
	void open(LPCTSTR file) {
		HRESULT hr = SHCreateStreamOnFile(file, STGM_READ, &m_fs);
		if(FAILED(hr)) throw xml_error("unable to open file", hr);

		hr = CreateXmlReader(__uuidof(IXmlReader), (void**)&m_reader, NULL);
		if(FAILED(hr)) throw xml_error("unable to create xml reader", hr);

		hr = m_reader->SetInput(m_fs);
		if(FAILED(hr)) throw xml_error("unable to set reader input", hr);
	}

	// closes the reader.
	void close() {
		m_reader.Release();
		m_fs.Release();
	}

	// sets a property on the reader.
	// see IXmlReader::SetProperty for possible values.
	void set_property(UINT prop, LONG_PTR val) {
		HRESULT hr = m_reader->SetProperty(prop, val);
		if(FAILED(hr)) throw xml_error("unable to set property", hr);
	}

	// sets if the reader should process DTDs for entity substitution.
	// otherwise, exceptions will be thrown if a DTD is encountered.
	void process_dtd(bool process = true) {
		set_property(XmlReaderProperty_DtdProcessing, process ? DtdProcessing_Parse : DtdProcessing_Prohibit);
	}

	// if entity depth gets higher than the limit, an exception will be thrown.
	// setting this to 0 will allow an unlimited depth.
	void max_entityexpansion(unsigned int limit = 0) {
		set_property(XmlReaderProperty_MaxEntityExpansion, limit);
	}

	// reads the next node in the stream.
	node_type read() {
		XmlNodeType nt;

		HRESULT hr = m_reader->Read(&nt);
		if(FAILED(hr)) throw xml_error("unable to read", hr);

		return static_cast<node_type>(nt);
	}

	// returns true on empty elements (ie, <foo/>)
	bool empty_element() {
		return m_reader->IsEmptyElement() != FALSE;
	}

	// skips the current element and all its sub-nodes.
	void skip_element() {
		if(!m_reader->IsEmptyElement()) {
			unsigned int depth = 0;

			node_type type;
			while((type = read()) != xml_reader::none) {
				if(type == element) {
					if(!m_reader->IsEmptyElement()) {
						++depth;
					}
				}
				else if(type == endelement) {
					if(!depth--) return;
				}
			}
		}
	}

	// gets the current entity depth.
	UINT depth() {
		UINT depth;

		HRESULT hr = m_reader->GetDepth(&depth);
		if(FAILED(hr)) throw xml_error("unable to get depth", hr);

		return depth;
	}

	// gets the current line number.
	UINT line_number() {
		UINT line;

		HRESULT hr = m_reader->GetLineNumber(&line);
		if(FAILED(hr)) throw xml_error("unable to get line number", hr);

		return line;
	}

	// gets the current column number.
	UINT column_number() {
		UINT col;

		HRESULT hr = m_reader->GetLinePosition(&col);
		if(FAILED(hr)) throw xml_error("unable to get line number", hr);

		return col;
	}

	// gets the number of attributes in the current element.
	UINT attribute_count() {
		UINT count;

		HRESULT hr = m_reader->GetAttributeCount(&count);
		if(FAILED(hr)) throw xml_error("unable to get line number", hr);

		return count;
	}

	// gets the local name of the node.
	// if len is specified it gets the length of the returned string.
	const wchar_t* local_name(UINT *len = NULL) {
		const wchar_t *str;

		HRESULT hr = m_reader->GetLocalName(&str, len);
		if(FAILED(hr)) throw xml_error("unable to get value", hr);

		return str;
	}

	// gets the value of the node.
	// if len is specified it gets the length of the returned string.
	const wchar_t* value(UINT *len = NULL) {
		const wchar_t *str;

		HRESULT hr = m_reader->GetValue(&str, len);
		if(FAILED(hr)) throw xml_error("unable to get value", hr);

		return str;
	}

	// moves to an attribute by name.
	bool move_to_attribute(const wchar_t *attr, const wchar_t *ns = NULL) {
		HRESULT hr = m_reader->MoveToAttributeByName(attr, ns);
		if(hr == S_OK) return true;
		else if(hr == S_FALSE) return false;
		else throw xml_error("unable to move to attribute", hr);
	}

	// moves to the first attribute.
	bool move_to_first_attribute() {
		HRESULT hr = m_reader->MoveToFirstAttribute();
		if(hr == S_OK) return true;
		else if(hr == S_FALSE) return false;
		else throw xml_error("unable to move to attribute", hr);
	}

	// moves to the next attribute.
	bool move_to_next_attribute() {
		HRESULT hr = m_reader->MoveToNextAttribute();
		if(hr == S_OK) return true;
		else if(hr == S_FALSE) return false;
		else throw xml_error("unable to move to attribute", hr);
	}

	// moves to the element of the current attribute.
	void move_to_element() {
		HRESULT hr = m_reader->MoveToElement();
		if(FAILED(hr)) throw xml_error("unable to move to element", hr);
	}

private:
	// noncopyable.
	xml_reader(const xml_reader&);
	void operator=(const xml_reader&);

	CComPtr<IStream> m_fs;
	CComPtr<IXmlReader> m_reader;
};

class xml_writer {
public:
	enum standalone_type {
		omit = XmlStandalone_Omit,
		no = XmlStandalone_No,
		yes = XmlStandalone_Yes
	};

	xml_writer() {}
	xml_writer(LPCTSTR file) { open(file); }
	~xml_writer() { close(); }

	void open(LPCTSTR file) {
		HRESULT hr = SHCreateStreamOnFile(file, STGM_WRITE | STGM_CREATE, &m_fs);
		if(FAILED(hr)) throw xml_error("unable to open file", hr);

		hr = CreateXmlWriter(__uuidof(IXmlWriter), (void**)&m_writer, NULL);
		if(FAILED(hr)) throw xml_error("unable to create xml reader", hr);

		hr = m_writer->SetOutput(m_fs);
		if(FAILED(hr)) throw xml_error("unable to set reader input", hr);
	}

	void close() {
		m_writer.Release();
		m_fs.Release();
	}

	void set_property(UINT prop, LONG_PTR val) {
		HRESULT hr = m_writer->SetProperty(prop, val);
		if(FAILED(hr)) throw xml_error("unable to set property", hr);
	}

	void indent(bool indent = true) {
		set_property(XmlWriterProperty_Indent, indent ? TRUE : FALSE);
	}

	void comment(const wchar_t *value) {
		HRESULT hr = m_writer->WriteComment(value);
		if(FAILED(hr)) throw xml_error("unable to write comment", hr);
	}

	void start_document(standalone_type standalone = omit) {
		HRESULT hr = m_writer->WriteStartDocument((XmlStandalone)standalone);
		if(FAILED(hr)) throw xml_error("unable to write document start", hr);
	}

	void end_document() {
		HRESULT hr = m_writer->WriteEndDocument();
		if(FAILED(hr)) throw xml_error("unable to write document end", hr);
	}

	void start_element(const wchar_t *local, const wchar_t *prefix = 0, const wchar_t *nsuri = 0) {
		HRESULT hr = m_writer->WriteStartElement(prefix, local, nsuri);
		if(FAILED(hr)) throw xml_error("unable to write element start", hr);
	}

	void end_element() {
		HRESULT hr = m_writer->WriteEndElement();
		if(FAILED(hr)) throw xml_error("unable to write element end", hr);
	}

	void string(const wchar_t *str) {
		HRESULT hr = m_writer->WriteString(str);
		if(FAILED(hr)) throw xml_error("unable to write string", hr);
	}

	void element_string(const wchar_t *local, const wchar_t *value, const wchar_t *prefix = 0, const wchar_t *nsuri = 0) {
		if(value && value[0]) {
			HRESULT hr = m_writer->WriteElementString(prefix, local, nsuri, value);
			if(FAILED(hr)) throw xml_error("unable to write element string", hr);
		}
		else {
			start_element(local, prefix, nsuri);
			end_element();
		}
	}
	
	void attribute(const wchar_t *local, const wchar_t *value, const wchar_t *prefix = 0, const wchar_t *nsuri = 0) {
		HRESULT hr = m_writer->WriteAttributeString(prefix, local, nsuri, value);
		if(FAILED(hr)) throw xml_error("unable to write element string", hr);
	}

private:
	// noncopyable.
	xml_writer(const xml_writer&);
	void operator=(const xml_writer&);

	CComPtr<IStream> m_fs;
	CComPtr<IXmlWriter> m_writer;
};
