/*

		   _      _                 _____       _____
	 /\   | |    | |          /\   |_   _|     / ____|
	/  \  | |    | |  ______ /  \    | |______| |     _ __  _ __
   / /\ \ | |    | | |______/ /\ \   | |______| |    | '_ \| '_ \
  / ____ \| |____| |____   / ____ \ _| |_     | |____| |_) | |_) |
 /_/    \_\______|______| /_/    \_\_____|     \_____| .__/| .__/
													 | |   | |
													 |_|   |_|

*
*   很高兴您的使用
* 
*	I'm glad you're using it
*
* ====================================================================================================
*
*   声明/开发者的话：
*   1. 开发者并非是AI领域（专业）的人，能力有限，望您海涵我的不足
*	2. 开发者正在求职（专业：计算机科学与技术），如果您愿意为我提供一个机会（岗位），可通过下方邮箱联系
*   3. 开源协议： MIT
*	
*	本库在线文档: https://ai-cpp-docsify.cpluscottage.top/
*	开发者个人博客: https://blog.wang-sz.cn
*	反馈/催更/交流邮箱: about@wang-sz.cn
* 
*   如果本库对您有所帮助，您不妨给个star支持一下，您的star是我最大的动力！
*/


#ifndef _ALL_AI_HPP_
#define _ALL_AI_HPP_

#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <regex>
#include <initializer_list>
#include <atomic>

#include <curl/curl.h>
#include <functional>

#include "nlohmann/json.hpp"

// Windows
#if ((defined(_WIN32) || defined(_WIN64)) && defined(_MSC_VER))
#define WIN_MSVC_VER 0L
#include <windows.h>
#include <strsafe.h>

// linux
#elif __linux__ 
#define LINUX_VER 1L
#include <stdlib.h>
#include <string.h>
#endif

namespace ALL_AI
{

	// HTTP方法枚举，目前仅支持基于libcurl的会话
	enum class HttpMethod {
		POST,
		GET
	};

	// 错误抛出方式
	enum class ALL_AI_ErrorThrow {
		ALL_AI_PRINT_ERROR,			// 通过打印错误信息
		ALL_AI_CALLBACK_FUNCTION,	// 通过回调函数返回错误信息
		ALL_AI_EXCEPTION_THROWING,	// 通过抛出异常的方式返回错误信息
		ALL_AI_NO_ERROR_THROW		// 不抛出错误
	};

	class ThrowError {
	public:
		ThrowError() {};
		~ThrowError() {};

		void SetThrowErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_function);

	protected:
		ALL_AI_ErrorThrow m_ErrorThrow = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW;
		std::function<void(const std::string_view& message)> m_callback_function;
	};

	/*
	============================================================================
	Function: SetThrowErrorCallbackFunction
	Description: 设置错误抛出的回调函数
	Parameters:
		- std::function<void(const std::string_view& message)> callback_function: 一个接受错误信息的回调函数
	Return: 无返回值
	============================================================================
	*/
	void ThrowError::SetThrowErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_function)
	{
		if(this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION && 
			callback_function != nullptr)
		{
			this->m_callback_function = callback_function;
		}
		return;
	}

	// 请求构建策略
	class IRequestBuilderStrategy : public ThrowError {
	public:
		virtual ~IRequestBuilderStrategy() = default;
		virtual nlohmann::json GetBuilder() = 0;
		virtual void ClearBuilder() = 0;
		virtual nlohmann::json GetEmptyBuilder() = 0;
	};

	// 响应解析策略
	class IResponseParserStrategy : public ThrowError {
	public:
		virtual ~IResponseParserStrategy() = default;
		virtual void Parse(const nlohmann::json& response) = 0;
		virtual void Parse(const std::string& response) = 0;
		virtual nlohmann::json GetData() = 0;
	};

	// Json操作相关的类和函数
	namespace JsonOperator {
		// JSON请求构建器
		class JsonRequestBuilder : public IRequestBuilderStrategy {

		public:

			/*
			 ============================================================================
			 Function: GetBuilder
			 Description: 获取json构建器
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回json
			 ============================================================================
			*/
			virtual nlohmann::json GetBuilder() override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_request);
				return this->m_request_json;
			}

			/*
			 ============================================================================
			 Function: ClearBuilder
			 Description: 清空json
			 Parameters:
				 - 无参数: 无释义
			 Return: 无返回值
			 ============================================================================
			*/
			virtual void ClearBuilder() override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_request);
				this->m_request_json.clear();
			}

			/*
			 ============================================================================
			 Function: GetEmptyBuilder
			 Description: 获取空json
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回一个空json对象
			 ============================================================================
			*/
			virtual nlohmann::json GetEmptyBuilder() override
			{
				return nlohmann::json{};
			}

			// 设置json某个字段值
			template <typename _T_Value, typename... Args>
			bool SetValue(_T_Value, Args... _keys);

		private:
			// 设置json某个字段的值：递归的结束层
			template <typename T>
			bool _setValue(nlohmann::json& _json, T&& val, const std::string& key);

			//设置json某个字段的值：递归中间层
			template <typename T, typename... Args>
			bool _setValue(nlohmann::json& _json, T&& val, const std::string& first, Args&&... rest);

		private:
			nlohmann::json m_request_json;

			mutable std::mutex m_mutex_request;
		};

		/*
		 ============================================================================
		 Function: setValue
		 Description: 设置json某个字段指定的值 - 接口
		 Parameters:
			 - _T_Value: 需要设置的值
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 设置成功返回true，否则返回false
		 ============================================================================
		*/
		template <typename _T_Value, typename... Args>
		bool JsonRequestBuilder::SetValue(_T_Value _value, Args... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			static_assert((std::is_convertible_v<Args, std::string> && ...),
				"All keys must be convertible to string");
			return _setValue(this->m_request_json, _value, _keys...);
		}

		/*
		 ============================================================================
		 Function: _setValue(
		 Description: 设置json某个字段指定的值 - 接口的实现
		 Parameters:
			 - nlohmann::json: 一个json对象
			 - const T&: 需要设置的值
			 - const std::string&: 一个字符串，作为json的字段，作为字段的索引
		 Return: 设置成功返回true，否则返回false
		 ============================================================================
		*/

		template <typename T>
		bool JsonRequestBuilder::_setValue(nlohmann::json& _json, T&& val, const std::string& key)
		{
			_json[key] = std::forward<T>(val);
			return true;
		}

		/*
		 ============================================================================
		 Function: _setValue
		 Description: 设置json某个字段指定的值 - 接口中间层
		   通过递归方式，支持“深度路径”写入：例如
		   _setValue(j, 42, "a", "b", "c") 等价于 j["a"]["b"]["c"] = 42
		   仅当整条路径上的所有中间对象都已存在时才写入，否则放弃并返回 false。
		 Parameters:
		   - nlohmann::json _json : 待修改的 json 对象（按值传递，内部副本）
		   - const T& val         : 要写入的最终值
		   - const std::string& first : 路径上的第一个键
		   - Args... rest         : 剩余键（可变参数包），长度可为 0
		 Return:
		   - true  – 成功找到叶子节点并完成赋值
		   - false – 路径中任一中间节点不存在，或中途遇到非对象类型，写入失败
		 ============================================================================
		*/
		template <typename T, typename... Args>
		bool JsonRequestBuilder::_setValue(nlohmann::json& _json, T&& val, const std::string& first, Args&&... rest)
		{
			// 直接继续递归，避免拷贝
			return _setValue(_json[first], std::forward<T>(val), std::forward<Args>(rest)...);
		}

		// json解析策略
		class JsonResponceParser : public IResponseParserStrategy {
		public:

			/*
			 ============================================================================
			 Function: Parse
			 Description: json解析策略
			 Parameters:
				 - nlohmann::json: 一个json对象
			 Return: 无返回值
			 ============================================================================
			*/
			virtual void Parse(const nlohmann::json& response) override
			{
				this->m_response_json = response;
				return;
			}

			/*
			 ============================================================================
			 Function: Parse
			 Description: json解析策略
			 Parameters:
				 - const std::string&: 一个json字符串
			 Return: 无返回值
			 ============================================================================
			*/
			virtual void Parse(const std::string& response) override
			{
				try
				{
					this->m_response_json = std::move(nlohmann::json::parse(response));
				}
				catch (const nlohmann::json::exception& e)
				{
					std::string err = e.what();

					if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("JsonParser::Parse: " + err);
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						if (this->m_callback_function != nullptr)
						{
							this->m_callback_function(err);
							return;
						}
					}
				}
			}

			/*
			 ============================================================================
			 Function: GetData
			 Description: 获取json
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回一个json对象，表示解析后的数据
			 ============================================================================
			*/
			virtual nlohmann::json GetData() override
			{
				return this->m_response_json;
			}

			// 获取某个字段的值 - 重载
			template <typename _T_Type, typename... _Keys>
			_T_Type GetValue(_Keys... _keys);

		private:

			// 获取json某个字段的值，递归结束层
			template <typename _T_Type, typename _Key>
			_T_Type _getValue(const nlohmann::json& _json, _Key&& key);

			// 获取json某个字段的值，递归中间层
			template <typename _T_Type, typename _First, typename... Args>
			_T_Type _getValue(const nlohmann::json& _json, _First&& first, Args... rest);

		private:
			nlohmann::json m_response_json;
			std::mutex m_mutex_response;
		};

		/*
		 ============================================================================
		 Function: getValueFromeJson
		 Description: 获取json某个字段指定的值 - 接口
		 Parameters:
		   - Args...: 剩余键（可变参数包），长度可为 0，作为索引
		 Return: 获取成功返回指定类型的值，否则返回与一个空类型
		 ============================================================================
		*/
		template <typename _T_Type, typename... _Keys>
		_T_Type JsonResponceParser::GetValue(_Keys... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_response);
			return _getValue<_T_Type>(this->m_response_json, std::forward<_Keys>(_keys)...);
		}

		/*
		 ============================================================================
		 Function: _setValue
		 Description: 设置json某个字段指定的值 - 接口中间层
		 Parameters:
		   - nlohmann::json _json : 待修改的 json 对象（按值传递，内部副本）
		   - const std::string&: 指定的键
		 Return: 函数执行成功返回一个特化的值，否则返回空特化值
		 ============================================================================
		*/
		template <typename _T_Type, typename _Key>
		_T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _Key&& key)
		{
			if constexpr (std::is_integral_v<std::decay_t<_Key>>)
			{
				// 数组索引
				if (!_json.is_array() || key < 0 || key >= _json.size()) 
				{
					if(this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Array index out of bounds: " + std::to_string(key));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION && 
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Array index out of bounds: " + std::to_string(key));
						return _T_Type{};
					}
				}
				return _json.at(key);
			}
			else 
			{
				// 对象键
				if (!_json.contains(key)) 
				{
					if(this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Key not found: " + std::string(key));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Key not found: " + std::string(key));
						return _T_Type{};
					}
				}
				return _json.at(key);
			}
		}

		/*
		 ============================================================================
		 Function: _getValue
		 Description: 设置json某个字段指定的值 - 接口中间层
		   仅当整条路径上的所有中间对象都已存在时才写入，否则放弃并返回 false。
		 Parameters:
		   - nlohmann::json _json : 待获取值的 json 对象
		   - const std::string& first : 路径上的第一个键
		   - Args... rest         : 剩余键（可变参数包），长度可为 0
		 Return: 获取成功返回对应的类型的数据，否则返回一个空数据
		 ============================================================================
		*/
		template <typename _T_Type, typename _First, typename... Args>
		_T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			nlohmann::json next_json = nullptr;

			if constexpr (std::is_integral_v<std::decay_t<_First>>) 
			{
				// 数组索引
				if (!_json.is_array() || first < 0 || first >=_json.size()) 
				{
					if(this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Array index out of bounds");
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Array index out of bounds: " + std::to_string(first));
						return _T_Type{};
					}
				}
				next_json = _json.at(first);
			}
			else 
			{
				// 对象键
				if (!_json.contains(first)) 
				{
					if(this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Key not found: " + std::string(first));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Key not found: " + std::string(first));
						return _T_Type{};
					}
				}
				next_json = _json.at(first);
			}

			return _getValue<_T_Type>(next_json, std::forward<Args>(rest)...);
		}
	}

	// Json操作相关的工具类
	class JsonOperatorTools {
	public:

		// 角色枚举
		enum class Role {
			System,
			User,
			Assistant
		};

		/*
		 ============================================================================
		 Function: JsonOperatorTools
		 Description: 构造函数
		 Parameters:
			 - 无参数: 无释义
		 Return: 无返回值
		 ============================================================================
		*/
		JsonOperatorTools() {}

		/*
		 ============================================================================
		 Function: ~JsonOperatorTools
		 Description: 析构函数
		 Parameters:
			 - 无参数: 无释义
		 Return: 无返回值
		 ============================================================================
		*/
		~JsonOperatorTools() {}

		/*
		 ============================================================================
		 Function: GetMessagesArray
		 Description: 获取消息数组
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个消息数组
		 ============================================================================
		*/
		nlohmann::json::array_t GetMessagesArray()
		{
			return this->m_array;
		}

		/*
		 ============================================================================
		 Function: PushBack
		 Description: 向消息数组中添加消息
		 Parameters:
			 - _role: 消息角色
			 - _content: 消息内容
		 Return: 无返回值
		 ============================================================================
		*/
		void PushBackArray(const Role& _role, const std::string& _content)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_json);

			this->m_array.push_back({ {"role", RoleToString(_role)}, {"content", _content} });
			return;
		}

		/*
		 ============================================================================
		 Function: PopBack
		 Description: 从消息数组中删除最后一个消息
		 Parameters:
			 - 无参数: 无释义
		 Return: 无返回值
		 ============================================================================
		*/
		void PopBackArray()
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_json);

			this->m_array.pop_back();
			return;
		}

	private:

		/*
		 ============================================================================
		 Function: RoleToString
		 Description: 将角色转换为字符串
		 Parameters:
			 - Role: 角色
		 Return: 返回字符串
		 ============================================================================
		*/
		static std::string RoleToString(Role r)
		{
			switch (r)
			{
			case Role::System:
				return "system";
			case Role::User:
				return "user";
			case Role::Assistant:
				return "assistant";
			}
			return "";
		}

	private:
		nlohmann::json::array_t m_array;
		std::mutex m_mutex_json;
	};

	// 抽象HTTP传输接口，定义了发送HTTP请求的方法
	class IHttpTransport {
	public:
		IHttpTransport() = default;
		virtual ~IHttpTransport() = default;
		// 初始化HTTP传输接口，设置URL、API Key和错误抛出方式
		virtual bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
		// 发送HTTP请求
		virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;

		// 设置错误回调函数
		virtual void SetErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_func) = 0;
	};

	namespace HttpTransport
	{
		// 基于libcurl的HTTP传输实现
		class CurlHttpTransport final : public IHttpTransport {
		public:

			/*
			 ============================================================================
			 Function: CurlHttpTransport
			 Description: 构造函数
			 Parameters:
				 - 无参数: 无释义
			 Return: 无
			 ============================================================================
			*/
			CurlHttpTransport()
			{	
			}

			/*
			 ============================================================================
			 Function: ~CurlHttpTransport
			 Description: 析构函数
			 Parameters:
				 - 无参数: 无释义
			 Return: 无
			 ============================================================================
			*/
			~CurlHttpTransport()
			{
				if (this->m_curl != nullptr)
				{
					// 清理libcurl
					curl_easy_cleanup(this->m_curl);
				}
			}

			/*
			 ============================================================================
			 Function: Initialize
			 Description: 初始化HTTP传输接口
			 Parameters:
				 - const std::string& url: HTTP请求的URL
				 - const std::string& api_key: API密钥
				 - const ALL_AI_ErrorThrow all_ai_error_throw: 错误抛出方式
			 Return: 无
			 ============================================================================
			*/
			virtual bool Initialize(const std::string& url,
				const std::string& api_key,
				const ALL_AI_ErrorThrow all_ai_error_throw) override
			{
				if (url.empty() || api_key.empty())
				{
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("CurlHttpTransport: url or api_key is empty");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("CurlHttpTransport: url or api_key is empty");
					}
					return false;
				}
				this->m_url = url;
				this->m_key = api_key;
				this->m_error_throw = all_ai_error_throw;

				// 初始化 libcurl
				this->m_curl = curl_easy_init();
				if (!this->m_curl)
				{
					// 如果初始化失败，根据错误抛出方式处理错误
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("CurlHttpTransport: curl_easy_init failed");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("CurlHttpTransport: curl_easy_init failed");
					}
					return false;
				}

				if (this->m_curl)
				{
					// 设置URL
					curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(this->m_curl, CURLOPT_FOLLOWLOCATION, 1L);

					// 忽略SSL
					curl_easy_setopt(this->m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
					curl_easy_setopt(this->m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
				}
				return true;
			}

			/*
			 ============================================================================
			 Function: SendRequest
			 Description: 发送HTTP请求
			 Parameters:
			   - HttpMethod method: HTTP请求方法
			   - const nlohmann::json request_json: 一个指向nlohmann::json对象的指针，表示请求的JSON数据
			 Return: 返回一个nlohmann::json，表示回复内容
			 ============================================================================
			*/
			virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// 如果初始化失败，则在请求时返回空json
				if (this->m_curl == nullptr)
				{
					return nlohmann::json{};
				}
				if(method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "POST");
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "GET");
				}
				else
				{
					return nlohmann::json{};
				}

				struct curl_slist* headers = nullptr;
				headers = curl_slist_append(headers, "Accept: application/json");
				if (this->m_key.empty())
				{
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("CurlHttpTransport: api_key is empty");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("CurlHttpTransport: api_key is empty");
						return nlohmann::json{};
					}
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());
				headers = curl_slist_append(headers, "Content-Type: application/json");
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// 清理可能的残留标志
				// clear flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);

				// 设置请求数据
				std::string str_json = request_json.dump();
				if (method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, str_json.c_str());
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// 执行请求
				CURLcode res = curl_easy_perform(this->m_curl);
				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					curl_slist_free_all(headers); // Ensure we free headers
					if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function(error_message);
					}
					return nlohmann::json{};
				}

				// Check HTTP response code
				long http_code = 0;
				curl_easy_getinfo(this->m_curl, CURLINFO_RESPONSE_CODE, &http_code);
				if (http_code < 200 || http_code >= 300)
				{
					std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
					curl_slist_free_all(headers);
					if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function(error_message);
						return nlohmann::json{};
					}
				}

				// Check if response is empty
				if (str_Buffer.empty())
				{
					curl_slist_free_all(headers);
					if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("Error: Session: Empty response from server");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("Error: Session: Empty response from server");
						return nlohmann::json{};
					}
				}

				// To JSON Data
				nlohmann::json json_result;
				try
				{
					json_result = nlohmann::json::parse(str_Buffer);
				}
				catch (const nlohmann::json::parse_error & e)
				{
					std::string error_message = "Error: Session: JSON parse failed. Response: " + str_Buffer + ", Error: " + e.what();
					curl_slist_free_all(headers);
					if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function(error_message);
						return nlohmann::json{};
					}
				}

				// Clean up headers
				curl_slist_free_all(headers);
				return json_result;
			}

			/*
			 ============================================================================
			 Function: SetErrorCallbackFunction
			 Description: 设置错误回调函数
			 Parameters:
			   - std::function<void(const std::string&)> callback: 一个接受错误信息的回调函数
			 Return: 无返回值
			 ============================================================================
			*/
			virtual void SetErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_func) override
			{
				this->m_callback_function = callback_func;
			}

			/*
			 ============================================================================
			 Function: SetErrorThrow
			 Description: 设置错误抛方式
			 Parameters:
			   - 
			 Return: 无
			 ============================================================================
			*/
			virtual void SetErrorThrow(ALL_AI_ErrorThrow error_throw) 
			{ 
				this->m_error_throw = error_throw;
			}

			/*
			 ============================================================================
			 Function: SetErrorThrowCallbackFunction
			 Description: 设置错误抛出回调函数
			 Parameters:
			   - std::function<void(const std::string_view& message)> callback_func: 一个接受错误信息的回调函数，函数参数为一个字符串视图，表示错误信息
			 Return: 无
			 ============================================================================
			*/
			void SetErrorThrowCallbackFunction(std::function<void(const std::string_view& message)> callback_func)
			{
				this->m_callback_function = std::move(callback_func);
				return;
			}
		
		private:

			/*
			 ============================================================================
			 Function: WriteCallback
			 Description: libcurl写数据回调函数，将下载的数据追加到用户指定的std::string中
			 Parameters:
			   - contents: 指向接收到的数据缓冲区
			   - size: 每个数据块的字节大小
			   - nmemb: 数据块的个数
			   - userp: 用户自定义指针，此处指向用于存储数据的std::string对象
			 Return: 返回实际处理的数据总字节数(size*nmemb)。若返回值与预期不符，libcurl会判定为错误并中止传输
			 ============================================================================
			*/
			static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
			{
				size_t totalSize = size * nmemb;
				userp->append(static_cast<char*>(contents), totalSize);
				return totalSize;
			}

		private:

			CURL* m_curl = nullptr;

			std::mutex m_mutex_curl_request;

			std::string m_url;	// API - URL
			std::string m_key;		// API - Key

			ALL_AI_ErrorThrow m_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW;

			std::function<void(const std::string_view& message)> m_callback_function = nullptr;
		};
	}

	class AI {
	public:

		/*
		 ============================================================================
		 Function: AI
		 Description: 构造函数
		 Parameters:
			 - 无参数: 无释义
		 Return: 无
		 ============================================================================
		*/
		explicit AI() {};

		/*
		 ============================================================================
		 Function: AI
		 Description: 构造函数
		 Parameters:
			 - std::shared_ptr<IHttpTransport> transport: 一个共享指针，指向一个实现了IHttpTransport接口的对象，用于处理HTTP请求
			 - const std::string& url: 一个字符串，表示API站的URL
			 - const std::string& api_key: 一个字符串，表示API站的API Key
			 - const ALL_AI_ErrorThrow all_ai_error_throw: 一个枚举值，表示错误抛出方式
		 Return: 无
		 ============================================================================
		*/
		explicit AI(std::shared_ptr<IHttpTransport> transport, 
			const std::string& url, 
			const std::string& api_key, 
			const ALL_AI_ErrorThrow all_ai_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW) :
			m_transport(std::move(transport)),
			m_url(url),
			m_api_key(api_key),
			m_error_throw(all_ai_error_throw) {}

		/*
		 ============================================================================
		 Function: ~AI
		 Description: 析构函数
		 Parameters:
			 - 无参数: 无释义
		 Return: 无
		 ============================================================================
		*/
		~AI() {};

		/*
		 ============================================================================
		 Function: SetErrorThrowCallbackFunction
		 Description: 设置错误抛出回调函数
		 Parameters:
			 - std::function<void(const std::string_view& message)> callback_func: 一个接受错误信息的回调函数，函数参数为一个字符串视图，表示错误信息
		 Return: 无返回值
		 ============================================================================
		*/
		bool SetErrorThrowCallbackFunction(std::function<void(const std::string_view& message)> callback_func)
		{
			// 设置错误抛出方式为回调函数，并保存回调函数
			if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION && 
				callback_func != nullptr)
			{
				this->m_callback_function = std::move(callback_func);
				if (this->m_transport)
				{
					this->m_transport->SetErrorCallbackFunction(callback_func);
				}
				return true;
			}
			else
			{
				return false;
			}
			return false;
		}

		/*
		 ============================================================================
		 Function: SetURL
		 Description: 设置API站的URL
		 Parameters:
			 -  const std::string&: 一个字符串，表示API站的URL
		 Return: 无返回值
		 ============================================================================
		*/
		void SetURL(const std::string& url)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_config);
			this->m_url = url;
			return;
		}

		/*
		 ============================================================================
		 Function: SetKey
		 Description: 设置API密钥
		 Parameters:
			 - const std::string&: 一个字符串，表示API密钥
		 Return: 无返回值
		 ============================================================================
		*/
		void SetKey(const std::string& key)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_config);
			this->m_api_key = key;
			return;
		}

		/*
		 ============================================================================
		 Function: SetHttpTransport
		 Description: 设置HTTP传输接口
		 Parameters:
			 - 
		 Return: 
		 ============================================================================
		*/
		void SetHttpTransport(std::shared_ptr<IHttpTransport> transport)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_config);
			// 检查传入的传输接口是否为空
			if (transport == nullptr)
			{
				return;
			}
			this->m_transport = std::move(transport);
			return;
		}

		 /*
		 ============================================================================
		 Function: InitAI
		 Description: 初始化AI，主要是进行一些必要的设置和准备工作，例如初始化HTTP传输接口、设置错误抛出方式等
		 Parameters: 
		     - 无参数: 无释义
		 Return: 返回一个布尔值，表示初始化是否成功。如果初始化成功，返回true；如果初始化失败，返回false
		 ============================================================================
		*/
		bool InitAI()
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_ai_init);

			// 如果初始化过则直接返回false，表示不需要重复初始化
			if (this->m_initialized == true)
			{
				return false;
			}
			// 设置构建器与解析器的错误抛出方式
			if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION && 
				this->m_callback_function != nullptr)
			{
				// 这里不需要加锁，因为InitAI不与SendRequest并发（由用户保证或m_initialized标志）
				// 且Builder/Parser内部有锁
				this->m_builder.SetThrowErrorCallbackFunction(this->m_callback_function);
				this->m_parser.SetThrowErrorCallbackFunction(this->m_callback_function);
			}

			// 获取配置的副本进行初始化
			std::string url_copy;
			std::string key_copy;
			std::shared_ptr<IHttpTransport> transport_copy;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				url_copy = this->m_url;
				key_copy = this->m_api_key;
				transport_copy = this->m_transport;
			}

			if (transport_copy)
			{
				this->m_initialized = transport_copy->Initialize(url_copy, key_copy, this->m_error_throw);
			}
			return this->m_initialized;
		}

		bool ReloadAI(std::string url = "",
			std::string api_key = "", 
			std::shared_ptr<IHttpTransport> transport = std::make_shared<HttpTransport::CurlHttpTransport>())
		{
			// 获取配置锁，更新配置
			std::lock_guard<std::mutex> lock(this->m_mutex_config);

			if (!url.empty())
			{
				this->m_url = url;
			}
			if (!api_key.empty())
			{
				this->m_api_key = api_key;
			}
			if (transport != nullptr)
			{
				this->m_transport = std::move(transport);
			}
			
			// 重新初始化Transport
			if (this->m_transport)
			{
				return this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw);
			}
			return false;
		}

		/*
		 ============================================================================
		 Function: SendRequest
		 Description: 发送HTTP请求，主要是将用户的请求数据转换为JSON格式，并通过HTTP传输接口发送给服务器，然后接收服务器的回复并返回给用户
		 Parameters:
			 - HttpMethod method: HTTP请求方法(1.POST 2.GET)
			 - const nlohmann::json request_json: 一个nlohmann::json对象，表示请求的JSON数据
		 Return: 返回一个nlohmann::json对象，表示服务器的回复内容。如果请求发送失败或服务器回复无效，返回一个空的nlohmann::json对象
		 ============================================================================
		*/
		nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json)
		{
			std::shared_ptr<IHttpTransport> transport_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
			}

			// 如果HTTP传输接口未设置，根据错误抛出方式处理错误
			if (!transport_local)
			{
				if(this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
				{
					throw std::runtime_error("AI: HTTP transport is not set");
				}
				else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION && 
					this->m_callback_function != nullptr)
				{
					this->m_callback_function("AI: HTTP transport is not set");
				}
				return nlohmann::json{};
			}
			
			// 这里的 SendRequest 是线程安全的（CurlHttpTransport 已加锁）
			// 这里的 Parse 也是线程安全的（JsonResponceParser 已加锁）
			nlohmann::json result = transport_local->SendRequest(method, request_json);
			this->m_parser.Parse(result);
			return this->m_parser.GetData();
		}

		/*
		 ============================================================================
		 Function: SendRequest_POST
		 Description: 发送POST请求
		 Parameters:
			 - const nlohmann::json request_json: 一个nlohmann::json对象，表示请求的JSON数据
		 Return: 返回一个nlohmann::json对象，表示服务器的回复内容。如果请求发送失败或服务器回复无效，返回一个空的nlohmann::json对象
		 ============================================================================
		*/
		nlohmann::json SendRequest_POST(const nlohmann::json request_json)
		{
			return SendRequest(HttpMethod::POST, request_json);
		}

		/*
		 ============================================================================
		 Function: SendRequest_GET
		 Description: 发送GET请求
		 Parameters:
			 - const nlohmann::json request_json: 一个nlohmann::json对象，表示请求的JSON数据
		 Return: 返回一个nlohmann::json对象，表示服务器的回复内容。如果请求发送失败或服务器回复无效，返回一个空的nlohmann::json对象
		 ============================================================================
		*/
		nlohmann::json SendRequest_GET(const nlohmann::json request_json)
		{
			return SendRequest(HttpMethod::GET, request_json);
		}

		/*
		 ============================================================================
		 Function: SendRequestFromBuilder_Get
		 Description: 发送GET请求
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个nlohmann::json对象，表示服务器的回复内容。如果请求发送失败或服务器回复无效，返回一个空的nlohmann::json对象
		 ============================================================================
		*/
		nlohmann::json SendRequestFromBuilder_Get()
		{
			nlohmann::json _json{};
			return SendRequest(HttpMethod::GET, _json);
		}

		/*
		 ============================================================================
		 Function: SendRequestFromBuilder_Post
		 Description: 发送Post请求 - 使用构建器
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个nlohmann::json对象，表示服务器的回复内容。如果请求发送失败或服务器回复无效，返回一个空的nlohmann::json对象
		 ============================================================================
		*/
		nlohmann::json SendRequestFromBuilder_Post()
		{
			// GetBuilder() 内部已加锁，返回副本
			return SendRequest(HttpMethod::POST, this->m_builder.GetBuilder());
		}
		
		/*
		 ============================================================================
		 Function: GetBuilder
		 Description: 获取构建器
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个构建器引用
		 ============================================================================
		*/
		JsonOperator::JsonRequestBuilder& GetBuilder()
		{
			// 不需要加锁，因为m_builder是成员变量，地址不变
			// 且JsonRequestBuilder的方法是线程安全的
			return this->m_builder;
		}

		/*
		 ============================================================================
		 Function: GetBuilderData
		 Description: 获取构建器Json数据
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个构建器中的json数据
		 ============================================================================
		*/
		nlohmann::json GetBuilderData()
		{
			return this->m_builder.GetBuilder();
		}

		/*
		 ============================================================================
		 Function: GetParser
		 Description: 获取一个解析器
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个解析器的引用
		 ============================================================================
		*/
		JsonOperator::JsonResponceParser& GetParser()
		{
			return this->m_parser;
		}

		/*
		 ============================================================================
		 Function: GetTools
		 Description: 获取工具类
		 Parameters:
			 - 无参数: 无释义
		 Return: 返回一个工具类的引用，工具类中包含了一些常用的JSON操作工具，
					例如ChatTool等，可以帮助用户更方便地构建请求和解析响应
		 ============================================================================
		*/
		JsonOperatorTools& GetTools()
		{
			return this->m_tools;
		}

	private:

		std::string m_url;	// API - URL
		std::string m_api_key;	// API - Key

		ALL_AI_ErrorThrow m_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW;		// 错误抛出方式

		std::shared_ptr<IHttpTransport> m_transport;	// HTTP传输接口
		std::function<void(const std::string_view& message)> m_callback_function = nullptr;		// 错误回调函数

		std::mutex m_mutex_config;		// 配置互斥锁（保护 URL, Key, Transport）
		std::mutex m_mutex_ai_init;		// AI初始化互斥锁

		JsonOperator::JsonRequestBuilder m_builder;
		JsonOperator::JsonResponceParser m_parser;
		JsonOperatorTools m_tools;

		bool m_initialized = false;	// AI是否已初始化
	};

}

#define ALL_AI_TOOL_MESSAGE_ROLE_USER		(ALL_AI::JsonOperatorTools::Role::User)
#define ALL_AI_TOOL_MESSAGE_ROLE_ASSISTANT	(ALL_AI::JsonOperatorTools::Role::Assistant)
#define ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM		(ALL_AI::JsonOperatorTools::Role::System)

#endif