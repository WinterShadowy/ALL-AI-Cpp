/*
*
*   很高兴您的使用
*   在开发过程中，我希望您能简单理解一些概念
*   当然，这些概念是由开发者自己定义的
*   content:	内容，它指的是服务器回复的内容，它是一个json对象
*   reply:		回复，它指的是用户所需要的回复
*
*	I'm glad you're using it
*	During the development process, I hope you can have a simple understanding of some concepts
*	Of course, these concepts are defined by the developers themselves
*	Content: It refers to the content replied by the server, which is a JSON object
*	Reply: reply, it refers to the response that the user needs
*
* ====================================================================================================
*
*   声明/开发者的话：
*   1. 开发者不会在这个文件中使用和收集任何您的信息，开发者也并不是AI领域的专业的人士
*   2. 开发者是一位在读计科专业学生，并非是业内从业者，能力实在有限，请您海涵我的不足
*   3. 开发者的个人网站：https://blog.wang-sz.cn ，您可以向我反馈或交流，期盼您的到来
*/

#ifndef _ALL_AI_HPP_
#define _ALL_AI_HPP_

#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <curl/curl.h>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <regex>
#include <initializer_list>

// Windows
#if ((defined(_WIN32) || defined(_WIN64)) && defined(_MSC_VER))
#define WIN_MSVC_VER 0L
#include <windows.h>
#include <strsafe.h>
#elif __linux__ // linux
#define LINUX_VER 1L
#include <stdlib.h>
#include <string.h>
#endif

#define ROLE_USER		"user"
#define ROLE_SYSTEM		"system"
#define ROLE_ASSISTANT	"assistant"

#define ROLE_USER_W			L"user"
#define ROLE_SYSTEM_W		L"system"
#define ROLE_ASSISTANT_W	L"assistant"

#define AI_COMPLETIONS	0U
#define AI_EDITS		1U
#define AI_MODEL_LIST	2U
#define AI_CHAT			3U
#define AI_IMAGES		4U
#define AI_FILE			5U
#define AI_EMBEDDINGS	6U
#define AI_FINETUNES	7U
#define AI_AUDIO		8U
#define AI_VIDEO		9U
#define AI_MODERATION	10U
#define AI_SELF_DEFINED 20U

typedef unsigned int UINT;

namespace _AI {
	using json = nlohmann::json;
	// character set
	namespace _characters {
#ifdef WIN_MSVC_VER
		inline std::wstring MultiToWide_Interface_Cpp(const std::string& str, int nCodePage = CP_UTF8)
		{
			int size = MultiByteToWideChar(nCodePage, 0, str.c_str(), -1, NULL, 0);
			std::wstring wstr(size, 0);
			MultiByteToWideChar(nCodePage, 0, str.c_str(), -1, &wstr[0], size);
			return wstr;
		}

		inline std::string WideToMulti_Interface_Cpp(const std::wstring& str, int nCodePage = CP_UTF8)
		{
			int size = WideCharToMultiByte(nCodePage, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
			std::string mstr(size, 0);
			WideCharToMultiByte(nCodePage, 0, str.c_str(), -1, &mstr[0], size, NULL, NULL);
			return mstr;
		}

		inline int MultiToWide_Interface_C(const char* szSrc, wchar_t* szBuffer, int bufferSize, int nCodePage = CP_UTF8)
		{
			int size = MultiByteToWideChar(nCodePage, 0, szSrc, -1, NULL, 0);
			if (szBuffer == NULL)
				return size;

			// The situation where the target string buffer is insufficient
			if (size > bufferSize)
				return -1;
			MultiByteToWideChar(nCodePage, 0, szSrc, -1, szBuffer, bufferSize);
			return size;
		}

		inline int WideToMulti_Interface_C(const wchar_t* szSrc, char* szBuffer, int bufferSize, int nCodePage = CP_UTF8)
		{
			int size = WideCharToMultiByte(nCodePage, 0, szSrc, -1, NULL, 0, NULL, NULL);
			if (szBuffer == NULL)
				return size;

			if (size > bufferSize)
				return -1;
			WideCharToMultiByte(nCodePage, 0, szSrc, -1, szBuffer, bufferSize, NULL, NULL);
			return size;
		}
#elif LINUX_VER
		inline size_t MultiToWide_Interface_Cpp(std::string str, std::wstring& targetStrBuffer)
		{
			size_t needSize = mbstowcs(NULL, str.c_str(), 0);
			if (needSize == SIZE_MAX)
				return (size_t)(-1);

			targetStrBuffer.resize(needSize + 1);
			wchar_t* targetStr = (wchar_t*)malloc(sizeof(wchar_t) * (needSize + 1));
			wmemset(targetStr, 0, sizeof(wchar_t) * (needSize + 1));
			if (targetStr == NULL)
				return (size_t)(-1);
			size_t ret = mbstowcs(targetStr, str.c_str(), needSize);
			targetStrBuffer = targetStr;
			free(targetStr);
			return ret;
		}

		inline size_t WideToMulti_Interface_Cpp(std::wstring& str, std::string& targetStrBuffer)
		{
			size_t needSize = wcstombs(NULL, str.c_str(), 0);
			if (needSize == SIZE_MAX)
				return (size_t)(-1);

			targetStrBuffer.resize(needSize + 1);
			char* targetStr = (char*)malloc(sizeof(char) * (needSize + 1));
			memset(targetStr, 0, sizeof(char) * (needSize + 1));
			if (targetStr == NULL)
				return (size_t)(-1);
			size_t ret = wcstombs(targetStr, str.c_str(), needSize);
			targetStrBuffer = targetStr;
			free(targetStr);
			return ret;
		}
#endif    
	}
	namespace CharSet = _AI::_characters;

	// JSON choices
	typedef struct Json_Choices {
		struct _Message {
			std::string role;
			std::string content;
		}Message;
		std::string finish_reason;
		unsigned int index = 0;
	}Json_Choices;

	class content {
	public:
		content() {}
		content(const json::array_t arr_Replies)
		{
			this->arr_replies_ = arr_Replies;
			AnalysisJsonArray();
		}
		~content() {}

		template <typename _T>
		_T Reply(unsigned int index = 0)
		{
			if constexpr (std::is_same_v<_T, std::string>)
			{
				return this->ToString(index);
			}
			else if constexpr (std::is_same_v<_T, std::wstring>)
			{
				return this->ToWString(index);
			}
			else
			{
				return _T();
			}
		}

		size_t GetMaxIndex()
		{
			return this->arr_json_choices_.size() - 1;
		}

		bool ContentEmpty()
		{
			return this->arr_json_choices_.empty();
		}

		// 获取回复数量
		size_t GetContentSize()
		{
			return this->arr_json_choices_.size();
		}

		// Retrieve URLs from replies, and if the index is an invalid value, return a total number of URLs
		template <typename _T>
		_T GetURLFromReply(int index = 0)
		{
			// 检查类型的一致性
			static_assert(std::is_same_v<_T, std::string> || std::is_same_v<_T, std::wstring> || std::is_same_v<_T, int>,
				"GetURLFromReply supports only std::string and std::wstring.");
			// Get all URLs from replies
			this->arr_urls_ = GetAllURL_FromReplies<std::vector<std::string> >();

			if constexpr (std::is_same_v<_T, int>)
			{
				return this->arr_urls_.size();
			}
			else if constexpr (std::is_same_v<_T, std::string>)
			{
				if (index < 0 || index > this->arr_urls_.size())
				{
					return std::move(std::string(""));
				}
				else
				{
					return this->arr_urls_[index];
				}
			}
			else if constexpr (std::is_same_v<_T, std::wstring>)
			{
				if (index < 0 || index > this->arr_urls_.size())
				{
					return std::move(std::wstring(L""));
				}
				else
				{
					return StringConversion<std::wstring>(this->arr_urls_[index]);
				}
			}
			else
			{
				return _T();
			}
		}

		template<typename _OutputContainer>
		_OutputContainer GetAllURL_FromReplies()
		{
			this->arr_urls_.clear();
			// OutputContainer must have value_type of std::string or std::wstring
			static_assert(
				std::is_same<typename _OutputContainer::value_type, std::string>::value ||
				std::is_same<typename _OutputContainer::value_type, std::wstring>::value,
				"OutputContainer must have value_type of std::string or std::wstring"
				);
			for (int i = 0; i < this->arr_json_choices_.size(); i++)
			{
				std::vector<std::string> url_buffer = GetURLFromReply(this->arr_json_choices_[i].Message.content);
				std::copy(url_buffer.begin(), url_buffer.end(), std::back_inserter(this->arr_urls_));
			}
			_OutputContainer urls;

			// 将回复中的 URL 添加到输出容器
			for (const std::string& url : this->arr_urls_)
			{
				urls.insert(urls.end(), url);
			}
			return urls;
		}

		template <typename _T>
		_T GetRole(unsigned int index = 0)
		{
			if constexpr (std::is_same_v<_T, std::string>)
			{
				return GetRoleA(index);
			}
			else if constexpr (std::is_same_v<_T, std::wstring>)
			{
				return GetRoleW(index);
			}
			else
			{
				return _T();
			}
		}

		template <typename _T>
		_T GetFinishReason(unsigned int index = 0)
		{
			if constexpr (std::is_same_v<_T, std::string>)
			{
				return GetFinishReasonA(index);
			}
			else if constexpr (std::is_same_v<_T, std::wstring>)
			{
				return GetFinishReasonW(index);
			}
			else
			{
				return _T();
			}
		}

	private:
		std::vector<std::string> GetURLFromReply(std::string markdownMessage)
		{
			const std::regex urlPattern(R"((http|https)://[^\s/$.?#].[^\s]*)");
			std::vector<std::string> urls;
			std::smatch match;

			// 创建一个 iterator 来搜索匹配
			std::string::const_iterator searchStart(markdownMessage.cbegin());
			while (std::regex_search(searchStart, markdownMessage.cend(), match, urlPattern))
			{
				urls.push_back(match[0].str().substr(0, match[0].length() - 1));
				searchStart = match.suffix().first; // 更新搜索开始位置
			}
			return urls;
		}

		template <typename _T_Return, typename _T_Parameter>
		_T_Return StringConversion(_T_Parameter str)
		{
			static_assert((std::is_same_v<_T_Return, std::string> || std::is_same_v<_T_Return, std::wstring>) &&
				(std::is_same_v<_T_Parameter, std::string>) || (std::is_same_v<_T_Parameter, std::wstring>),
				"StringConversion supports only std::string and std::wstring.");

			if constexpr (std::is_same_v<_T_Return, std::string> && std::is_same_v<_T_Parameter, std::wstring>)
			{
				return CharSet::WideToMulti_Interface_Cpp(str);
			}
			else if constexpr (std::is_same_v<_T_Return, std::wstring> && std::is_same_v<_T_Parameter, std::string>)
			{
				return CharSet::MultiToWide_Interface_Cpp(str);
			}
			else
			{
				return str;
			}
		}

		std::string ToString(unsigned int index = 0)
		{
			if (index >= arr_replies_.size())
			{
				return std::string("");
			}
			return this->arr_json_choices_[index].Message.content;
		}

		std::wstring ToWString(unsigned int index = 0)
		{
			if (index >= arr_replies_.size())
			{
				return std::wstring(L"");
			}
			std::wstring wstr;
#ifdef WIN_MSVC_VER
			wstr = CharSet::MultiToWide_Interface_Cpp(this->arr_json_choices_[index].Message.content);
#elif LINUX_VER
			CharSet::MultiToWide_Interface_Cpp(this->arr_json_choices_[index].Message.content, wstr);
#endif
			return wstr;
		}

		// Get role
		std::string GetRoleA(unsigned int index = 0)
		{
			if (index >= arr_replies_.size())
			{
				return std::string("");
			}
			return this->arr_json_choices_[index].Message.role;
		}
		std::wstring GetRoleW(unsigned int index = 0)
		{
			if (index >= arr_replies_.size())
			{
				return std::wstring(L"");
			}
			std::wstring wstr;
#ifdef WIN_MSVC_VER
			wstr = CharSet::MultiToWide_Interface_Cpp(this->arr_json_choices_[index].Message.role);
#elif LINUX_VER
			CharSet::MultiToWide_Interface_Cpp(this->arr_json_choices_[index].Message.role, wstr);
#endif
			return wstr;
		}

		// Obtain the reason for completion
		std::string GetFinishReasonA(unsigned int index = 0)
		{
			if (index >= arr_replies_.size())
			{
				return std::string("");
			}
			return this->arr_json_choices_[index].finish_reason;
		}

		std::wstring GetFinishReasonW(unsigned int index = 0)
		{
			if (index >= arr_replies_.size())
			{
				return std::wstring(L"");
			}
			std::wstring wstr;
#ifdef WIN_MSVC_VER
			wstr = CharSet::MultiToWide_Interface_Cpp(this->arr_json_choices_[index].finish_reason);
#elif LINUX_VER
			CharSet::MultiToWide_Interface_Cpp(this->arr_json_choices_[index].finish_reason, wstr);
#endif
			return wstr;
		}

		friend std::ostream& operator<<(std::ostream& os, const content& content_json_array)
		{
			os << content_json_array.arr_json_choices_[0].Message.content;
			return os;
		}

		friend std::wostream& operator<<(std::wostream& os, const content& content_json_array)
		{
			std::wstring wstr;
#ifdef WIN_MSVC_VER
			wstr = CharSet::MultiToWide_Interface_Cpp(content_json_array.arr_json_choices_[0].Message.content);
#elif LINUX_VER
			CharSet::MultiToWide_Interface_Cpp(content_json_array.arr_json_choices_[0].Message.content, wstr);
#endif
			os << wstr;
			return os;
		}
		// Analysis
		void AnalysisJsonArray()
		{
			for (const auto& choice : this->arr_replies_)
			{
				Json_Choices json_choice;
				json_choice.Message.role = choice.at("message").at("role").get<std::string>();
				json_choice.Message.content = choice.at("message").at("content").get<std::string>();
				json_choice.finish_reason = choice.at("finish_reason").get<std::string>();
				json_choice.index = choice.at("index").get<unsigned int>();
				arr_json_choices_.push_back(json_choice);
			}
		}
	private:
		json::array_t arr_replies_;
		std::vector<Json_Choices> arr_json_choices_;
		std::vector<std::string> arr_urls_;
	};

	// JSON request parser
	class JsonResponseParser {
	public:
		JsonResponseParser() {}
		JsonResponseParser(const json& json) : json_(json) {}
		~JsonResponseParser() {}

		void SetJson(const json& json_reply)
		{
			this->json_ = json_reply;
			return;
		}

		content GetContent()
		{
			json::array_t arr_Replies = json_.at("choices");
			return content(arr_Replies);
		}

		template <typename _T>
		_T GetJsonValue(std::string key)
		{
			// Determine whether the field exists
			if (!json_.contains(key) || json_.at(key).type() == json::value_t::null)
			{
				return _T();
			}

			// Get the value corresponding to the field
			try
			{
				return json_.at(key).get<_T>();
			}
			catch (const json::exception&)
			{
				return _T();
			}
		}

		void PrintJson_CMD(unsigned int count = 0)
		{
			std::cout << json_.dump(count) << std::endl;
			return;
		}
		json GetJson()
		{
			return json_;
		}

		std::string ToString(const json& json)
		{
			std::string str = json.dump();
			return str;
		}
		std::wstring ToWString(const json& json)
		{
			std::string str = json.dump();
			std::wstring wstr;
#ifdef WIN_MSVC_VER
			wstr = CharSet::MultiToWide_Interface_Cpp(str);
#elif LINUX_VER
			CharSet::MultiToWide_Interface_Cpp(str, wstr);
#endif
			return wstr;
		}

		// IntegerToString template function
		template <typename T>
		std::string IntegerToString(T value)
		{
			try {
				return std::to_string(value);
			}
			catch (const std::exception& e) {
				return std::string("");
			}
		}

		template <typename T>
		std::wstring IntegerToWString(T value)
		{
			try {
				return std::to_wstring(value);
			}
			catch (const std::exception& e) {
				std::wstring(L"");
			}
		}

		template<class _T>
		_T GetJsonValue(std::initializer_list<std::string> keyList)
		{
			return GetJsonValue(keyList).get<_T>();
		}
	private:
		// 可变键列表
		decltype(auto) GetJsonValue(std::initializer_list<std::string> keyList)
		{
			json const* cur = &json_;          // 从根开始
			for (auto& k : keyList)
			{
				if (!cur->is_object() || !cur->count(k))
					throw std::runtime_error("key not found");
				cur = &(*cur)[k];              // 深入下一层
			}
			return *cur;
		}
	private:
		json json_;
	};
	// json builder - base-class
	class JsonRequestBuilder {
	public:
		JsonRequestBuilder()
		{
			//this->m_messages_ = json::array();
			this->m_json_["messages"] = json::array();
			//this->m_json_["messages"].push_back({{ "role", "user" }, { "content", "You are a helpful assistant." }});
			this->m_json_["stream"] = false;
		}
		~JsonRequestBuilder() {}

		template <typename _T>
		void Insert(const std::string& str_key, const _T& _t_value)
		{
			this->m_json_[str_key] = _t_value;
			return;
		}

		// Send
		void Model(const std::string& str_model)
		{
			this->m_json_["model"] = str_model;
			return;
		}

		void N(unsigned int n_n)
		{
			this->m_json_["n"] = n_n;
			return;
		}

		void Prompt(const std::string& str_prompt)
		{
			this->m_json_["prompt"] = str_prompt;
			return;
		}

		void Stop(const std::string& str_stop)
		{
			this->m_json_["stop"] = str_stop;
			return;
		}

		void MaxTokens(unsigned int un_max_tokens)
		{
			this->m_json_["max_tokens"] = un_max_tokens;
			return;
		}

		void Temperature(float f_temperature)
		{
			this->m_json_["temperature"] = f_temperature;
			return;
		}

		void TopP(unsigned int un_top_p)
		{
			this->m_json_["top_p"] = un_top_p;
			return;
		}

		void Stream(bool b_stream)
		{
			this->m_json_["stream"] = b_stream;
			return;
		}

		void Push_Back_Message(const std::string& str_role, const std::string& str_content)
		{
			this->m_messages_.push_back({ {"role", str_role }, {"content", str_content} });
			return;
		}
		void Messages(const json::array_t& _arr_messages)
		{
			this->m_messages_ = _arr_messages;
			return;
		}

		void FrequencyPenalty(const float f_frequency_penalty)
		{
			this->m_json_["frequency_penalty"] = f_frequency_penalty;
			return;
		}
		void PresencePenalty(const float f_presence_penalty)
		{
			this->m_json_["presence_penalty"] = f_presence_penalty;
			return;
		}

		void User(const std::string& str_user)
		{
			this->m_json_["user"] = str_user;
			return;
		}

		template <typename _T_Param>
		void AddCustomFieldToJson(const std::string& str_key, const _T_Param& _t_value)
		{
			this->m_json_.insert(str_key, _t_value);
			return;
		}

		template <typename ... Args>
		void AddCustomFieldToJson(int nKeyValuePairNumber, Args... args)
		{
			if (nKeyValuePairNumber <= 0 || nKeyValuePairNumber > 16)
				return;
			HandleKeyValuePairs(nKeyValuePairNumber, 0, args...);
			return;
		}

		const json GetRequestJson()
		{
			this->m_json_["messages"] = this->m_messages_;
			return m_json_;
		}

		void PrintRequestJson_CMD(unsigned int un_Count = 0)
		{
			std::cout << m_json_.dump(un_Count) << std::endl;
			return;
		}
	private:
		template <typename _T_KeyValuePair>
		void HandleKeyValuePair(const _T_KeyValuePair& _t_key_value_pair)
		{
			if constexpr (std::is_same_v<typename _T_KeyValuePair::first_type, std::string>)
			{
				this->m_json_[_t_key_value_pair.first] = _t_key_value_pair.second;
			}
			return;
		}

		void HandleKeyValuePairs(const int nRecursionDepth, int nNowDepth)
		{
			return;
		}

		template <typename _T_KeyValuePair, typename... _T_Rest>
		void HandleKeyValuePairs(const int nRecursionDepth, int nNowDepth, const _T_KeyValuePair& _t_key_value_pair, const _T_Rest&... _t_rest)
		{
			if (nNowDepth >= nRecursionDepth)
				return;
			HandleKeyValuePair(_t_key_value_pair);
			// 递归处理其余键值对
			HandleKeyValuePairs(nRecursionDepth, nNowDepth + 1, _t_rest...);
		}

		// 特化为最后的基线，递归终止条件


	private:
		json m_json_;
		json::array_t m_messages_;
	};

	class AI;

#define AI_CUSTOM_REQUEST_POST	"POST"
#define AI_CUSTOM_REQUEST_GET	"GET"

	// Session
	class Session {
	public:
		Session() = delete;
		Session(const std::string& url, const std::string& key)
		{
			InitializeCURL(url, key);
		}
		~Session()
		{
			CleanupCURL();
		}
		json MakeRequest_GET()
		{
			std::lock_guard<std::mutex> lock(mutex_session_request_);
			return MakeRequest(json{}, AI_CUSTOM_REQUEST_GET);
		}

		json MakeRequest_POST(const json& _json)
		{
			std::lock_guard<std::mutex> lock(mutex_session_request_);
			return MakeRequest(_json, AI_CUSTOM_REQUEST_POST);
		}

	private:
		json MakeRequest(const json& _json, std::string str_custom_request)
		{
			if (curl_ == nullptr)
				return json{};

			curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, str_custom_request.c_str());
			struct curl_slist* headers = nullptr;
			headers = curl_slist_append(headers, "Accept: application/json");
			std::string authHeader = "Authorization: Bearer " + key_;
			headers = curl_slist_append(headers, authHeader.c_str());
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

			// 清理可能的残留标志
			// clear flags
			curl_easy_setopt(curl_, CURLOPT_POST, 0L);
			curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, nullptr);
			curl_easy_setopt(curl_, CURLOPT_NOBODY, 0L);

			std::string str_json = _json.dump();
			if (str_custom_request == AI_CUSTOM_REQUEST_POST)
			{
				curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, str_json.c_str());
			}
			else if (str_custom_request == AI_CUSTOM_REQUEST_GET)
			{
				curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
			}


			std::string str_Buffer;
			curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &str_Buffer);

			CURLcode res = curl_easy_perform(curl_);
			if (res != CURLE_OK)
			{
				std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
				curl_slist_free_all(headers); // Ensure we free headers
				throw std::runtime_error(error_message);
			}

			// Check HTTP response code
			long http_code = 0;
			curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
			if (http_code < 200 || http_code >= 300)
			{
				std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
				curl_slist_free_all(headers);
				throw std::runtime_error(error_message);
			}

			// Check if response is empty
			if (str_Buffer.empty())
			{
				curl_slist_free_all(headers);
				throw std::runtime_error("Error: Session: Empty response from server");
			}

			// To JSON Data
			json json_result;
			try
			{
				json_result = json::parse(str_Buffer);
			}
			catch (const json::parse_error& e)
			{
				std::string error_message = "Error: Session: JSON parse failed. Response: " + str_Buffer + ", Error: " + e.what();
				curl_slist_free_all(headers);
				throw std::runtime_error(error_message);
			}
			// Clean up headers
			curl_slist_free_all(headers);
			return json_result;
		}
		void InitializeCURL(const std::string& url, const std::string& key)
		{
			this->url_base_ = url;
			this->key_ = key;

			if (!InitCURL())
			{
				throw std::runtime_error("Error: Session: InitCURL failed");
			}
			if (!SetRequestUrl(url))
			{
				throw std::runtime_error("Error: Session: SetRequestUrl failed");
			}
			if (!IgnoreSSL())
			{
				throw std::runtime_error("Error: Session: IgnoreSSL failed");
			}
		}
		void CleanupCURL()
		{
			if (this->curl_ != nullptr)
			{
				curl_easy_cleanup(this->curl_);
			}
		}
		bool InitCURL()
		{
			this->curl_ = curl_easy_init();
			return this->curl_ != nullptr; // More direct method
		}
		bool SetRequestUrl(const std::string& url)
		{
			if (curl_ == nullptr)
				return false;
			curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
			return true;
		}
		bool IgnoreSSL()
		{
			if (curl_ == nullptr)
				return false;
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
			return true;
		}
		static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
		{
			size_t totalSize = size * nmemb;
			userp->append(static_cast<char*>(contents), totalSize);
			return totalSize;
		}
	private:
		CURL* curl_ = nullptr;
		std::string url_base_;
		std::string key_;
		std::mutex mutex_session_request_;
	};

	class Completions {
	public:
		Completions()
		{
			this->m_json_json_Builder_ = new JsonRequestBuilder();
			this->m_json_json_Parser_ = new JsonResponseParser();
		}
		~Completions()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class Edits {
	public:
		Edits()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Edits() {}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class ModelList {
	public:
		ModelList()
		{
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~ModelList()
		{
			delete this->m_json_json_Parser_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		void GetModelInfo(void (*function)(nlohmann::json))
		{
			function(this->m_json_json_Parser_->GetJson());
			return;
		}

		const json GetRequestBuilderJson()
		{
			return json{};
		}

	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}

		std::string GetCustomRequest()
		{
			return AI_CUSTOM_REQUEST_GET;
		}
	private:
		JsonResponseParser* m_json_json_Parser_;	// parser
	};

	class Chat {
	public:
		Chat()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Chat()
		{
			delete m_json_json_Builder_;
			delete m_json_json_Parser_;
		}

		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class Images {
	public:
		Images()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Images()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		// 设置json
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class File {
	public:
		File()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~File()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class Embeddings {
	public:
		Embeddings()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Embeddings()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class FineTunes {
	public:
		FineTunes()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~FineTunes()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class Audio {
	public:
		Audio()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Audio()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class Video {
	public:
		Video()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Video()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class Moderation {
	public:
		Moderation()
		{
			this->m_json_json_Builder_ = new (std::nothrow) JsonRequestBuilder();
			this->m_json_json_Parser_ = new (std::nothrow) JsonResponseParser();
		}
		~Moderation()
		{
			delete this->m_json_json_Builder_;
			delete this->m_json_json_Parser_;
		}
		JsonRequestBuilder* SetJson()
		{
			return m_json_json_Builder_;
		}
		JsonResponseParser& GetReply()
		{
			return *m_json_json_Parser_;
		}

		const json GetRequestBuilderJson()
		{
			return m_json_json_Builder_->GetRequestJson();
		}
	private:
		friend class AI;
		void SetResponseParser(json& json_response)
		{
			m_json_json_Parser_->SetJson(json_response);
		}
	private:
		JsonRequestBuilder* m_json_json_Builder_;	// builder
		JsonResponseParser* m_json_json_Parser_;	// Parser
	};

	class AI {
	public:
		AI(const std::string& url_base, const std::string& api_key, bool b_UseContext = false)
		{
			InitFactory(url_base, api_key);
			return;
		}

		~AI()
		{
			DestoryFactory();
		}
		AI& Instance(const std::string& url_base, const std::string& api_key)
		{
			static AI instance_(url_base, api_key);
			return instance_;
		}
		// Send user`s request for json-data
		json SendRequest(const json json_request, UINT unFunction)
		{
			json json_response;
			switch (unFunction)
			{
			case AI_COMPLETIONS:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_completions_->SetResponseParser(json_response);
				break;
			case AI_EDITS:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_edits_->SetResponseParser(json_response);
				break;
			case AI_MODEL_LIST:
				json_response = this->m_p_session_->MakeRequest_GET();
				this->m_model_list_->SetResponseParser(json_response);
				break;
			case AI_CHAT:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_chat_->SetResponseParser(json_response);
				break;
			case AI_IMAGES:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_images_->SetResponseParser(json_response);
				break;
			case AI_FILE:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_file_->SetResponseParser(json_response);
				break;
			case AI_EMBEDDINGS:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_embeddings_->SetResponseParser(json_response);
				break;
			case AI_FINETUNES:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_fine_tunes_->SetResponseParser(json_response);
				break;
			case AI_AUDIO:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_audio_->SetResponseParser(json_response);
				break;
			case AI_VIDEO:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_video_->SetResponseParser(json_response);
				break;
			case AI_MODERATION:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				this->m_moderation_->SetResponseParser(json_response);
				break;
			case AI_SELF_DEFINED:
				json_response = this->m_p_session_->MakeRequest_POST(json_request);
				break;
			}
			return json_response;
		}
		void Reinitialize(const std::string& url_base, const std::string& api_key)
		{
			if (this->m_p_session_)
			{
				delete this->m_p_session_;
				this->m_p_session_ = nullptr;
			}
			this->m_p_session_ = new (std::nothrow) Session(url_base, api_key);
			return;
		}

		json SendRequest_GET()
		{
			return this->m_p_session_->MakeRequest_GET();
		}

		json SendRequest_POST(const json& json_request)
		{
			return this->m_p_session_->MakeRequest_POST(json_request);
		}

		Completions* Set_Completions()
		{
			return this->m_completions_;
		}

		Edits* Set_Edits()
		{
			return this->m_edits_;
		}

		ModelList* Set_ModelList()
		{
			return this->m_model_list_;
		}

		Chat* Set_Chat()
		{
			return this->m_chat_;
		}

		Images* Set_Images()
		{
			return this->m_images_;
		}

		File* Set_File()
		{
			return this->m_file_;
		}

		Embeddings* Set_Embeddings()
		{
			return this->m_embeddings_;
		}

		FineTunes* Set_FineTunes()
		{
			return this->m_fine_tunes_;
		}

		Audio* Set_Audio()
		{
			return this->m_audio_;
		}

		Video* Set_Video()
		{
			return this->m_video_;
		}

		Moderation* Set_Moderation()
		{
			return this->m_moderation_;
		}

	private:
		// Initilize
		void InitFactory(const std::string& str_url_base, const std::string& str_api_key)
		{
			this->m_p_session_ = new (std::nothrow) Session(str_url_base, str_api_key);
			m_completions_ = new Completions();
			m_edits_ = new Edits();
			m_model_list_ = new ModelList();
			m_chat_ = new Chat();
			m_images_ = new Images();
			m_file_ = new File();
			m_embeddings_ = new Embeddings();
			m_fine_tunes_ = new FineTunes();
			m_audio_ = new Audio();
			m_video_ = new Video();
			m_moderation_ = new Moderation();
			return;
		}
		// destory
		void DestoryFactory()
		{
			delete this->m_p_session_;
			delete m_completions_;
			delete m_edits_;
			delete m_model_list_;
			delete m_chat_;
			delete m_images_;
			delete m_file_;
			delete m_embeddings_;
			delete m_fine_tunes_;
			delete m_audio_;
			delete m_video_;
			delete m_moderation_;
			return;
		}
	private:
		Session* m_p_session_ = nullptr;
		Completions* m_completions_ = nullptr;  // completion
		Edits* m_edits_ = nullptr;  // edit
		ModelList* m_model_list_ = nullptr;  // Get API-Station model list info
		Chat* m_chat_ = nullptr;  // chat
		Images* m_images_ = nullptr;  // images
		File* m_file_ = nullptr;  // file
		Embeddings* m_embeddings_ = nullptr;  // embedding
		FineTunes* m_fine_tunes_ = nullptr;	// fine-tune
		Audio* m_audio_ = nullptr;  // audio
		Video* m_video_ = nullptr;  // vudeo
		Moderation* m_moderation_ = nullptr;  // moderation
	};
}
namespace ALL_AI = _AI;

#endif