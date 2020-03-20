

#include <pugixml.hpp>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <direct.h>
#include <vector>
#include <fstream>
#include <map>



std::string stripChars(const std::string& aString)
{
	std::string ret;
	const char* start = aString.c_str();
	const char* end = aString.c_str() + aString.size();
	const char* tmp = NULL;

	while (*start == ' ' || *start == '\n' || *start == '\r' || *start == '\t')
		++start;

	while (end != start && (*end == '\n' || *end == '\r' || *end == '\0' || *end == ' ' || *end == '\t'))
		--end;

	if (start != end) {
		if (*start == '"' && *end == '"') {
			++start;
			--end;
		}

		for (tmp = start; tmp != end; ++tmp) {
			ret += *tmp;
		}
	}


	return ret;
}


static std::string _XMLSubDocument(const pugi::xml_node& aNode)
{
	std::string ret;

	for (auto& ch : aNode) {
		ret += ch.text().as_string();
		ret += _XMLSubDocument(ch);
	}

	ret = stripChars(ret);

	return ret;
}


class CElement {
public:
	std::string getSourceFile(void) const { return sourceFile_; }
	int getSourceLine(void) const { return sourceLine_; }
	std::string getLibrary(void) const { return library_; }
	std::string getDLL(void) const { return DLL_; }
	void setSourcePosition(const std::string& aFile, int aLine) 
	{
		sourceLine_ = aLine;		
		auto l = aFile.find_last_of('\\');
		if (l == std::string::npos)
			l = -1;

		sourceFile_ = aFile.substr(l + 1);

		return;
	}
	void setLinkRequirements(const std::string& aLibary, const std::string& aDLL)
	{
		library_ = aLibary;
		DLL_ = aDLL;

		return;
	}
	void loadSourceFile(const pugi::xml_node& aNode, std::vector<std::string>& aLines)
	{
		std::string fileName = aNode.attribute("source").as_string();

		if (!fileName.empty()) {
			std::ifstream sourceStream(fileName);
			std::string line;
			while (std::getline(sourceStream, line))
				aLines.push_back(line);

			sourceStream.close();
		}

		return;
	}
private:
	int sourceLine_;
	std::string sourceFile_;
	std::string library_;
	std::string DLL_;
};

class CMethodArgument : public CElement {
public:
	CMethodArgument(const std::string& aName, const std::string& aType, const pugi::xml_node & aText)
		: name_(aName), type_(aType), text_(aText), indirections_(0),
	isConst_(false), isLong_(false) { 
		size_t delimiter = type_.find('!');

		if (delimiter != std::string::npos) {
			std::string modifier = type_.substr(delimiter + 1);
			type_ = type_.substr(0, delimiter);
			while (modifier[modifier.size() - 1] == '*') {
				++indirections_;
				modifier.erase(modifier.end() - 1);
			}

			if (modifier == "System.Runtime.CompilerServices.IsConst")
				isConst_ = true;
			else if (modifier == "System.Runtime.CompilerServices.IsLong")
				isLong_ = true;
			else __debugbreak();
		}
	}
	pugi::xml_node getText(void) const { return text_; }
	std::string getName(void) const { return name_;; }
	std::string getType(void) const { return type_; }
	bool isConst(void) const { return isConst_; }
	bool isLong(void) const { return isLong_; }
	size_t getIndirections(void) const { return indirections_; }
private:
	std::string name_;
	std::string type_;
	pugi::xml_node text_;
	bool isConst_;
	bool isLong_;
	size_t indirections_;
};

class CMethod : public CElement {
public:
	CMethod(const pugi::xml_node& aNode, const std::string & aName)
		: returnType_("void")
	{
		std::string typeName;
		const char* tmp = NULL;
		const char* tmp2 = NULL;
		std::string text;

		tmp = aName.c_str();
		while (*tmp != '\0' && *tmp != '(')
			++tmp;
			
		name_ = std::string(aName.c_str(), tmp);
		if (*tmp == '(') {
			++tmp;
		}

		std::string source = aNode.attribute("source").as_string();
		int line = aNode.attribute("line").as_int();
		setSourcePosition(source, line);
		for (auto& n : aNode.children()) {
			std::string name = n.name();

			if (name == "summary")
				summary_ = n;
			else if (name == "returns") {
				returns_ = n;
				auto retType = n.attribute("type");
				if (!retType.empty())
					returnType_ = retType.as_string();
			} else if (name == "remarks")
				remarks_ = n;
			else if (name == "param") {
				tmp2 = tmp;
				while (*tmp != ',' && *tmp != ')')
					++tmp;

				typeName = std::string(tmp2, tmp);
				++tmp;
				args_.push_back(CMethodArgument(n.attribute("name").as_string(), typeName, n));
			}
		}

		return;
	}
	std::string getName(void) const { return name_; }
	pugi::xml_node getSummary(void) const { return summary_; }
	pugi::xml_node getReturns(void) const { return returns_; }
	std::string getReturnType(void) const { return returnType_; }
	pugi::xml_node getRemarks(void) const { return remarks_; }
	size_t argCount(void) const { return args_.size(); }
	const CMethodArgument& arg(const size_t index) const { return args_[index]; }
private:
	std::string name_;
	pugi::xml_node summary_;
	pugi::xml_node remarks_;
	pugi::xml_node returns_;
	std::string returnType_;
	std::vector<CMethodArgument> args_;
};

typedef enum _ETypeType {
	ttSimple,
	ttStruct,
	ttEnum,
	ttUnion,
} ETypeType, * PETypeType;

class CStructField : public CElement {
public:
	CStructField(ETypeType aParentType, const std::string& aName, const pugi::xml_node& aText)
		: parentType_(aParentType), value_(-1), name_(aName), text_(aText) {
		std::vector<std::string> sourceFileData;
		std::string sourceFile = aText.attribute("source").as_string();
		int sourceLine = aText.attribute("line").as_int();
		std::string line;

		loadSourceFile(aText, sourceFileData);
		line = sourceFileData[sourceLine - 1];
		while (line.find("///") != std::string::npos) {
			++sourceLine;
			line = sourceFileData[sourceLine - 1];
		}

		switch (parentType_) {
			case ttStruct:
			case ttUnion: {
				size_t endIndex = line.find(name_);
				size_t startIndex = 0;

				while (isspace(line[startIndex]))
					++startIndex;

				do {
					--endIndex;
				} while (isspace(line[endIndex]));
				
				type_ = line.substr(startIndex, endIndex - startIndex + 1);
				startIndex = line.find_first_of('[');
				endIndex = line.find_first_of(']');
				if (startIndex != std::string::npos &&
					endIndex != std::string::npos)
					arraySpecifier_ = line.substr(startIndex, endIndex - startIndex + 1);
			} break;
			case ttEnum: {
				auto startIndex = line.find_first_of('=');

				if (startIndex != std::string::npos) {
					do {
						++startIndex;
					} while (isspace(line[startIndex]));

					auto endIndex = startIndex;
					do {
						++endIndex;
					} while (endIndex < line.size() && isalnum(line[endIndex]));
				
					type_ = line.substr(startIndex, endIndex - startIndex);
					value_ = (int)std::strtol(type_.c_str(), NULL, 0);
				}
			}break;
		}

		setSourcePosition(sourceFile, sourceLine);
	}
	std::string getName(void) const { return name_; }
	std::string getType(void) const { return type_; }
	std::string getArraySpecifier(void) const { return arraySpecifier_; }
	int getValue(void) const { return value_; }
	void setValue(int aValue) { value_ = aValue; }
	pugi::xml_node getText(void) const { return text_; }
private:
	ETypeType parentType_;
	std::string name_;
	std::string type_;
	std::string arraySpecifier_;
	int value_;
	pugi::xml_node text_;
};

class CType : public CElement {
public:
	CType(const pugi::xml_node& aNode, const std::string & aName)
	{
		name_ = aName;
		auto sumNode = aNode.child("summary");
		std::vector<std::string> sourceFileData;

		if (!sumNode.empty())
			summary_ = sumNode;
		else summary_ = aNode;

		remarks_ = aNode.child("remarks");
		std::string source = aNode.attribute("source").as_string();
		int sourceLine = aNode.attribute("line").as_int();
		{
			std::string line;

			loadSourceFile(aNode, sourceFileData);
			line = sourceFileData[sourceLine - 1];
			while (line.find("///") != std::string::npos) {
				++sourceLine;
				line = sourceFileData[sourceLine - 1];
			}
			
			ETypeType types[] = {
				ttStruct,
				ttEnum,
				ttUnion,
			};
			std::string words[] = {
				"struct",
				"enum",
				"union",
			};

			setSourcePosition(source, sourceLine);
			type_ = ttSimple;
			for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
				auto index = line.find(words[i]);
				if (index != std::string::npos &&
					isspace(line[index - 1]) &&
					isspace(line[index + words[i].size()])) {
					type_ = types[i];
					break;
				}
			}
		}

		std::map<int, CStructField> lineMap;
		if (type_ == ttStruct) {
			auto tmp = aNode.next_sibling();
			do {
				std::string fullTypeName = tmp.attribute("name").as_string();
				if (fullTypeName.size() < 2 || fullTypeName[1] != ':' ||
					fullTypeName[0] != 'F')
					break;

				fullTypeName = fullTypeName.substr(2);
				if (fullTypeName.substr(0, name_.size()) != name_)
					break;

				fullTypeName = fullTypeName.substr(name_.size() + 1);
				CStructField cf = CStructField(type_, fullTypeName, tmp);
				lineMap.insert(std::make_pair(cf.getSourceLine(), cf));
				tmp = tmp.next_sibling();
			} while (!tmp.empty());
		}

		if (type_ == ttEnum) {
			auto tmp = aNode.previous_sibling();
			do {
				std::string fullTypeName = tmp.attribute("name").as_string();
				if (fullTypeName.size() < 2 || fullTypeName[1] != ':' ||
					fullTypeName[0] != 'F')
					break;

				fullTypeName = fullTypeName.substr(2);
				if (fullTypeName.find_first_of('.') != std::string::npos)
					break;
				
				CStructField cf = CStructField(type_, fullTypeName, tmp);
				lineMap.insert(std::make_pair(cf.getSourceLine(), cf));
				tmp = tmp.previous_sibling();
			} while (!tmp.empty());

			int enumValue = 0;
			for (auto& icf : lineMap) {
				CStructField& cf = icf.second;
				
				if (cf.getValue() == -1) {
					cf.setValue(enumValue);
					++enumValue;
				} else enumValue = cf.getValue() + 1;
			}

		}

		for (auto& icf : lineMap)
			fields_.push_back(icf.second);

		return;
	}
	std::string getName(void) const { return name_; }
	pugi::xml_node getSummary(void) const { return summary_; }
	pugi::xml_node getRemarks(void) const { return remarks_; }
	size_t getFieldCount(void) const { return fields_.size(); }
	CStructField getField(size_t index) const { return fields_[index]; }
	ETypeType getType(void) const { return type_; }
private:
	ETypeType type_;
	std::string name_;
	pugi::xml_node summary_;
	pugi::xml_node remarks_;
	std::vector<CStructField> fields_;

};


class COutputMd {
public:
	COutputMd(const std::map<std::string, CMethod*> & aMethods, const std::map<std::string, CType*> & aTypes, const std::string& aOutDir = ".")
		: outDir_(aOutDir), methods_(aMethods), types_(aTypes)
	{ }
	void getLinkInfo(const std::string& aLinkValue, std::string& aDescription, std::string& aTarget)
	{
		bool err = false;

		err = (aLinkValue.size() < 2 || aLinkValue[1] != ':');
		if (!err) {
			switch (aLinkValue[0]) {
				case 'M': {
					auto l = aLinkValue.find_first_of('(');
					if (l == std::string::npos)
						l = aLinkValue.size();

					auto methodName = aLinkValue.substr(2, l - 2);
					auto mit = methods_.find(methodName);
					err = (mit == methods_.cend());
					aDescription = methodName;
					if (!err)
						aTarget = "Function_" + methodName;
				} break;
				case 'T': {
					auto typeName = aLinkValue.substr(2);
					auto tit = types_.find(typeName);
					err = (tit == types_.cend());
					aDescription = typeName.substr(1);
					if (!err)
						aTarget = "Type_" + typeName;
				} break;
				case 'F': {
					auto fullName = aLinkValue.substr(2);
					auto l = fullName.find_first_of('.');
					if (l == std::string::npos)
						l = fullName.size();

					auto typeName = fullName.substr(0, l);
					auto tit = types_.find(typeName);
					err = (tit == types_.cend());
					aDescription = fullName.substr(1);
					if (!err)
						aTarget = "Type_" + typeName;
				} break;
				case '!': {
					auto name = aLinkValue.substr(2);
					auto mit = methods_.find(name);
					if (mit != methods_.cend()) {
						aDescription = name;
						aTarget = "Function_" + name;
					} else {
						auto fullName = name;
						auto l = fullName.find_first_of('.');
						if (l == std::string::npos)
							l = fullName.size();

						auto typeName = fullName.substr(0, l);
						auto tit = types_.find(typeName);
						err = (tit == types_.cend());
						aDescription = fullName.substr(1);
						if (!err)
							aTarget = "Type_" + typeName;
					}

				} break;
				default:
					err = true;
					break;
			}
		}

		if (err)
			fprintf(stderr, "Invalid link: %s\n", aLinkValue.c_str());

		return;
	}

	std::string OutputText(const pugi::xml_node& aRoot, std::map<std::string, std::string> & aSeeLinks)
	{
		std::string ret;

		for (auto& ch : aRoot) {
			if (ch.name() == NULL || *ch.name() == '\0') {
				ret += ch.text().as_string();
				ret += OutputText(ch, aSeeLinks);
			} else if (stricmp(ch.name(), "para") == 0) {
				ret += "\n\n" + OutputText(ch, aSeeLinks) + "\n\n";
			} else if (stricmp(ch.name(), "c") == 0) {
				ret += "`" + OutputText(ch, aSeeLinks) + "`";
			} else if (stricmp(ch.name(), "see") == 0) {
				std::string link = ch.attribute("cref").as_string();
				std::string description;
				std::string target;

				getLinkInfo(link, description, target);
				ret += "[" + description + "](" + target + ")";
				aSeeLinks.insert(std::make_pair(description, target));
			} else if (stricmp(ch.name(), "paramref") == 0) {
				ret += ch.attribute("name").as_string();
			} else if (stricmp(ch.name(), "list") == 0) {
				auto listType = ch.attribute("type").as_string();
				if (stricmp(listType, "table") == 0) {
					auto header = ch.child("listheader");
					if (!header.empty()) {
						auto hv = OutputText(header.child("term"), aSeeLinks);
						auto hd = OutputText(header.child("description"), aSeeLinks);
					
						ret = "\n| " + hv + " | " + hd + " |\n";
					} else {
						ret += "\n| Value | Description |\n";
					}

					ret += "|---|---|\n";
					for (auto& i : ch) {
						if (i.name() != NULL && stricmp(i.name(), "item") == 0) {
							auto hv = OutputText(i.child("term"), aSeeLinks);
							auto hd = OutputText(i.child("description"), aSeeLinks);

							ret += "| " + hv + " | " + hd + " |\n";
						}
					}
				}
			}
		}

		return ret;
	}
	int OutputMethod(const CMethod& aMethod)
	{
		int ret = 0;
		FILE* f = NULL;
		std::string fileName = outDir_ + "\\Function_" + aMethod.getName() + ".md";
		std::string tmp;
		std::map<std::string, std::string> seeList;

		f = fopen(fileName.c_str(), "w");
		if (f != NULL) {
			fprintf(f, "# %s function\n\n", aMethod.getName().c_str());
			tmp = OutputText(aMethod.getSummary(), seeList);
			fprintf(f, "## Summary\n\n%s\n\n", tmp.c_str());
			fprintf(f, "## Definition\n\n```c\n");
			fprintf(f, "%s cdecl %s(\n", aMethod.getReturnType().c_str(), aMethod.getName().c_str());
			for (size_t i = 0; i < aMethod.argCount(); ++i) {
				auto& a = aMethod.arg(i);

				fprintf(f, "    ");
				if (a.isConst())
					fprintf(f, "const ");

				if (a.isLong())
					fprintf(f, "long ");

				fprintf(f, "%s ", a.getType().c_str());
				for (size_t j = 0; j < a.getIndirections(); ++j)
					fprintf(f, "*");

				fprintf(f, "%s", a.getName().c_str());
				if (i != aMethod.argCount() - 1)
					fprintf(f, ",");

				fprintf(f, "\n");
				auto tit = types_.find(a.getType());
				if (tit != types_.cend())
					seeList.insert(std::make_pair(tit->second->getName(), "Type_" + tit->second->getName()));
			}

			fprintf(f, "   );\n```\n\n");
			if (aMethod.argCount() > 0) {
				fprintf(f, "## Parameters\n\n");
				for (size_t i = 0; i < aMethod.argCount(); ++i) {
					auto& a = aMethod.arg(i);
					tmp = OutputText(a.getText(), seeList);
					fprintf(f, "### %s\n\n%s\n\n", a.getName().c_str(), tmp.c_str());
				}
			}

			if (!aMethod.getReturns().empty()) {
				fprintf(f, "## Return Value\n\n");
				tmp = OutputText(aMethod.getReturns(), seeList);
				fprintf(f, "%s\n\n", tmp.c_str());
			}

			if (!aMethod.getRemarks().empty()) {
				fprintf(f, "## Remarks\n\n");
				tmp = OutputText(aMethod.getRemarks(), seeList);
				fprintf(f, "%s\n\n", tmp.c_str());
			}

			if (seeList.size() > 0) {
				fprintf(f, "## See also\n\n");
				for (auto& i : seeList) {
					fprintf(f, "* [%s](%s)\n", i.first.c_str(), i.second.c_str());
				}

				fprintf(f, "\n");
			}

			fprintf(f, "\n## Requirements\n\n");
			fprintf(f, "|   |   |\n");
			fprintf(f, "|---|---|\n");
			fprintf(f, "| **Header** | %s |\n", aMethod.getSourceFile().c_str());
			fprintf(f, "| **Library** | %s |\n", aMethod.getLibrary().c_str());
			fprintf(f, "| **DLL** | %s |\n", aMethod.getDLL().c_str());
			fprintf(f, "\n");

			fclose(f);
		} else ret = errno;

		return ret;
	}
	int OutputType(const CType& aType)
	{
		int ret = 0;
		FILE* f = NULL;
		std::string fileName = outDir_ + "\\Type_" + aType.getName() + ".md";
		std::string tmp;
		std::map<std::string, std::string> seeList;
		const char* words[] = {
			"type",
			"struct",
			"enum",
			"union",
		};

		f = fopen(fileName.c_str(), "w");
		if (f != NULL) {
			fprintf(f, "# %s %s\n\n", aType.getName().c_str(), words[aType.getType()]);
			tmp = OutputText(aType.getSummary(), seeList);
			fprintf(f, "## Summary\n\n%s\n\n", tmp.c_str());
			if (aType.getType() != ttSimple) {
				fprintf(f, "## Definition\n\n");
				fprintf(f, "```c\n");
				fprintf(f, "typedef %s %s {\n", words[aType.getType()], aType.getName().c_str());
				for (size_t i = 0; i < aType.getFieldCount(); ++i) {
					auto a = aType.getField(i);
					switch (aType.getType()) {
						case ttStruct:
						case ttUnion: {
							fprintf(f, "    %s %s%s;\n", a.getType().c_str(), a.getName().c_str(), a.getArraySpecifier().c_str());
							auto tit = types_.find(a.getType());
							if (tit != types_.cend())
								seeList.insert(std::make_pair(tit->second->getName(), "Type_" + tit->second->getName()));
						} break;
						case ttEnum:
							fprintf(f, "    %s = %i,\n", a.getName().c_str(), a.getValue());
							break;
					}
				}

				std::string baseName = aType.getName();
				if (baseName[0] == '_')
					baseName = baseName.substr(1);

				fprintf(f, "} %s, *P%s;\n```\n\n", baseName.c_str(), baseName.c_str());
				switch (aType.getType()) {
					case ttStruct:
					case ttUnion:
						fprintf(f, "## Members\n\n");
						break;
					case ttEnum:
						fprintf(f, "## Values\n\n");
						break;
				}

				for (size_t i = 0; i < aType.getFieldCount(); ++i) {
					auto a = aType.getField(i);
					fprintf(f, "### %s\n\n", a.getName().c_str());
					tmp = OutputText(a.getText(), seeList);
					fprintf(f, "%s\n\n", tmp.c_str());
				}
			}

			if (!aType.getRemarks().empty()) {
				fprintf(f, "## Remarks\n\n");
				tmp = OutputText(aType.getRemarks(), seeList);
				fprintf(f, "%s\n\n", tmp.c_str());
			}

			if (seeList.size() > 0) {
				fprintf(f, "## See also\n\n");
				for (auto& i : seeList) {
					fprintf(f, "* [%s](%s)\n", i.first.c_str(), i.second.c_str());
				}

				fprintf(f, "\n");
			}

			fprintf(f, "\n## Requirements\n\n");
			fprintf(f, "|   |   |\n");
			fprintf(f, "|---|---|\n");
			fprintf(f, "| **Header** | %s |\n", aType.getSourceFile().c_str());
			fprintf(f, "\n");

			fclose(f);
		} else ret = errno;

		return ret;
	}
	int OutputMethodList(void)
	{
		int ret = 0;
		FILE* f = NULL;
		std::string fileName = outDir_ + "\\FunctionList.md";

		f = fopen(fileName.c_str(), "w");
		if (f != NULL) {
			fprintf(f, "# Function list\n\n");
			for (auto& m : methods_)
				fprintf(f, "* [%s](Function_%s)\n", m.first.c_str(), m.first.c_str());

			fclose(f);
		}

		return ret;
	}
	int OutputTypeList(void)
	{
		int ret = 0;
		FILE* f = NULL;
		std::string fileName = outDir_ + "\\TypeList.md";

		f = fopen(fileName.c_str(), "w");
		if (f != NULL) {
			fprintf(f, "# Type list\n\n");
			for (auto& t : types_)
				fprintf(f, "* [%s](Type_%s)\n", t.first.c_str(), t.first.c_str());

			fclose(f);
		}

		return ret;
	}
	int OutputSidebar(void)
	{
		int ret = 0;
		FILE* f = NULL;
		std::string fileName = outDir_ + "\\_Sidebar.md";

		f = fopen(fileName.c_str(), "w");
		if (f != NULL) {
			fprintf(f, "### Functions\n");
			for (auto& m : methods_)
				fprintf(f, "* [%s](Function_%s)\n", m.first.c_str(), m.first.c_str());

			fprintf(f, "\n### Types\n");
			for (auto& t : types_)
				fprintf(f, "* [%s](Type_%s)\n", t.first.c_str(), t.first.c_str());

			fclose(f);
		}

		return ret;

		return ret;
	}
private:
	std::string outDir_;
	std::map<std::string, CMethod*> methods_;
	std::map<std::string, CType*> types_;
};


static std::string libraryName;
static std::string DLLName;
static std::map<std::string, CMethod*> methods;
static std::map<std::string, CType*> types;
static std::map<std::string, pugi::xml_document*> docs_;

static int _processXMLFile(const pugi::xml_document & aDoc)
{
	int ret = 0;
	CMethod* m = NULL;
	CType* t = NULL;
	std::string nodeName;
	std::string itemName;
	std::string sourceFile;
	int sourceLine = 0;

	for (pugi::xml_node& n : aDoc.child("doc").child("members")) {
		nodeName = n.attribute("name").as_string();
		if (nodeName.size() >= 2 && nodeName[1] == ':') {
			itemName = nodeName.substr(2);
			switch (nodeName[0]) {
				case 'M':
					m = new CMethod(n, itemName);
					m->setLinkRequirements(libraryName, DLLName);
					methods.insert(std::make_pair(m->getName(), m));
					break;
				case 'T': {
					t = new CType(n, itemName);
					types.insert(std::make_pair(t->getName(), t));
					auto baseName = t->getName();
					if (baseName[0] == '_')
						baseName = baseName.substr(1);

					types.insert(std::make_pair(baseName, t));
					baseName = "P" + baseName;
					types.insert(std::make_pair(baseName, t));
				} break;
				case 'F':
					break;
				default:
					std::cout << nodeName;
					break;
			}
		}
	}

	return ret;
}


int main(int argc, char* argv[])
{
	int ret = 0;

	if (argc < 4)
		return -1;

	libraryName = argv[1];
	DLLName = argv[2];
	for (int i = 3; i < argc; ++i) {
		pugi::xml_document* doc = NULL;

		fprintf(stderr, "Processing %s\n", argv[i]);
		doc = new pugi::xml_document();
		docs_.insert(std::make_pair(argv[i], doc));
		pugi::xml_parse_result result = doc->load_file(argv[i]);
		if (result)
			_processXMLFile(*doc);
	}

	_mkdir("..\\bin\\doc");
	COutputMd outMd(methods, types, "..\\bin\\doc");
	for (auto& m : methods)
		outMd.OutputMethod(*m.second);

	for (auto& t : types)
		outMd.OutputType(*t.second);

	outMd.OutputSidebar();

	return ret;
}
