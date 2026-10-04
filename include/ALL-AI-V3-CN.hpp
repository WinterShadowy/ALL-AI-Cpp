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
*		2.5 很幸运，开发者找到了工作。
*   3. 开源协议： MIT
*
*	本库在线文档:
*		https://doc.cpluscottage.top/web/#/642380673
*		https://ai-cpp-docsify.cpluscottage.top/(停用)
*	开发者个人博客: https://blog.wang-sz.cn
*	反馈/催更/交流邮箱: about@wang-sz.cn
*
*   如果本库对您有所帮助，您不妨给个star支持一下，您的star是我最大的动力！
* 
* ====================================================================================================
*
*   Developer's notes:
*   1. The developer is not an AI specialist, and my abilities are limited. Thank you for your understanding.
*   2. The developer is currently seeking a job (major: Computer Science and Technology). If you would like to offer an opportunity, please contact me via the email below.
*		2.5 Fortunately, the developer has found a job.
*   3. Open-source license: MIT
*
*	Online documentation: https://ai-cpp-docsify.cpluscottage.top/
*	Developer's blog: https://blog.wang-sz.cn
*   Feedback / updates / contact email: about@wang-sz.cn
*
*   If this library helps you, please consider giving it a star. Your support is my greatest motivation!
* 
* ====================================================================================================
* 
* ！！！Translation from KimiAI！！！
*
* ====================================================================================================
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
#include <type_traits>
#include <variant>
#include <fstream>
#include <iterator>
#include <cctype>
#include <unordered_set>

#include <curl/curl.h>
#include <functional>
#include <optional>

#include "nlohmann/json.hpp"

#if (defined(_WIN32) || defined(_WIN64))    // Windows
#ifndef __ALL_AI_SYSTEM_MARKER
#define __ALL_AI_SYSTEM_MARKER 0x80L	// bin: 1000 0000
#endif
#elif (defined(__linux__) || defined(__linux))		// Linux
#ifndef __ALL_AI_SYSTEM_MARKER
#define __ALL_AI_SYSTEM_MARKER 0x40L	// bin: 0100 0000
#endif
#else
#error "Unsupported operating system. This library only supports Windows and Linux."
#endif

// 操作系统：Windows
#if __ALL_AI_SYSTEM_MARKER >= 0x80L
#include <windows.h>
#include <strsafe.h>
// windows.h 中定义的 DELETE 宏与 HttpMethod::DELETE 枚举值冲突，此处取消定义
// 注意：如果用户代码在本头文件之后又包含了windows.h，需要自行再次 #undef DELETE
#if (defined(DELETE))
#undef DELETE
#endif

// Win32API: DeleteFile 与 删除API站上指定的文件函数冲突
// 如需使用Win32API删除文件，请在本头文件之后包含windows.h
#if (defined(DeleteFile))
#undef DeleteFile
#endif

#elif (__ALL_AI_SYSTEM_MARKER >= 0x40L && __ALL_AI_SYSTEM_MARKER < 0x80L)
#include <stdlib.h>
#include <string.h>
#endif

// 判断编译器
// MSVC
#if defined(_MSC_VER)
#define __ALL_AI_CXX_STANDARD _MSVC_LANG
// g++
#elif defined(__GNUC__)
#define __ALL_AI_CXX_STANDARD __cplusplus
#else
#error "Unsupported compiler. This library only supports MSVC and g++."
#endif

#if __ALL_AI_CXX_STANDARD >= 202002L
#define __ALL_AI_CPP_VERSION_20 20L
#elif __ALL_AI_CXX_STANDARD >= 201703L
#define __ALL_AI_CPP_VERSION_17 17L
#elif __ALL_AI_CXX_STANDARD >= 201402L
#define __ALL_AI_CPP_VERSION_14 14L
#elif __ALL_AI_CXX_STANDARD >= 201103L
#define __ALL_AI_CPP_VERSION_11 11L
#else
#error "The C++ version is too low. This library does not support this C++ standard."
#endif

#if defined (__ALL_AI_CPP_VERSION_20)
#define __ALL_AI_CXX_VERSION 20L
#elif defined (__ALL_AI_CPP_VERSION_17)
#define __ALL_AI_CXX_VERSION 17L
#elif defined (__ALL_AI_CPP_VERSION_14)
#define __ALL_AI_CXX_VERSION 14L
#elif defined (__ALL_AI_CPP_VERSION_11)
#define __ALL_AI_CXX_VERSION 11L
#endif

namespace ALL_AI
{
	// HTTP方法枚举，目前仅支持基于libcurl的会话
	enum class HttpMethod {
		POST,
		GET,
		DELETE
	};

	// 数据回调：收到响应数据块时逐块调用（用于音频流等二进制响应、SSE、大文件下载），
	// 返回值语义与libcurl写回调一致：返回已消费的字节数，不等于传入size时中止请求
	using DataCallback = std::function<size_t(const char* data, size_t size)>;

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


		/*
		============================================================================
		Function: SetThrowErrorCallbackFunction
		Description: 设置错误抛出的回调函数
		Parameters:
			- std::function<void(const std::string_view& message)> callback_function: 一个接受错误信息的回调函数
		Return: 无返回值
		============================================================================
		*/
		void SetThrowErrorCallbackFunction(
#if __ALL_AI_CXX_VERSION >= 17L
			std::function<void(const std::string_view& message)> callback_function
#elif __ALL_AI_CXX_VERSION >= 14L
			std::function<void(const std::string& message)> callback_function
#endif
		)
		{
			if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
				callback_function != nullptr)
			{
				this->m_callback_function = callback_function;
			}
			return;
		}
		/*
		============================================================================
		Function: DoErrorThrow
		Description: 执行错误抛出操作
		Parameters:
			- std::string_view message: 错误信息
		Return: 无返回值
		============================================================================
		*/
		void DoErrorThrow(
#if __ALL_AI_CXX_VERSION >= 17L
			std::string_view message
#elif __ALL_AI_CXX_VERSION >= 14L
			std::string message
#endif
		)
		{
			if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
			{
				throw std::runtime_error(std::string(message));
			}
			else if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
			{
				if (this->m_callback_function != nullptr)
				{
					this->m_callback_function(message);
				}
				else
				{
					std::cerr << "Error: No valid callback function set for error throwing." << std::endl;
					std::cerr << "Message: " << message << std::endl;
				}
			}
			else if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
			{
				std::cerr << message << std::endl;
			}
			return;
		}

	protected:
		ALL_AI_ErrorThrow m_error_throw_method = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW;
#if __ALL_AI_CXX_VERSION >= 17L
		std::function<void(const std::string_view& message)> m_callback_function;
#elif __ALL_AI_CXX_VERSION >= 14L
		std::function<void(const std::string& message)> m_callback_function;
#endif
	};

	// 请求构建策略
	class IRequestBuilderStrategy : virtual public ThrowError {
	public:
		virtual ~IRequestBuilderStrategy() = default;

		virtual nlohmann::json BuilderToJson() = 0;
		virtual void ClearBuilder() = 0;
		virtual nlohmann::json GetEmptyBuilder() = 0;
	};

	// 响应解析策略
	class IResponseParserStrategy : virtual public ThrowError {
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

			JsonRequestBuilder() {}

			virtual ~JsonRequestBuilder() override = default;

			/*
			 ============================================================================
			 Function: BuilderToJson
			 Description: 将构建器内容转换为json对象
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回nlohmann::json
			 ============================================================================
			*/
			virtual nlohmann::json BuilderToJson() override
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
			bool SetValue(_T_Value value, Args... keys);

			// 追加到数组（如果路径不存在则创建数组，如果存在但不是数组则失败），且在数组末尾追加
			template <typename _T_Value, typename... Args>
			bool ArrayPushBack(_T_Value value, Args... keys);

			// 删除数组末尾元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename... Args>
			bool ArrayDeleteBack(Args... keys);

			// 在数组头部追加元素（如果路径不存在则创建数组，如果存在但不是数组则失败）
			template <typename _T_Value, typename... Args>
			bool ArrayPushFront(_T_Value value, Args... keys);

			// 删除数组头部元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename... Args>
			bool ArrayDeleteFront(Args... keys);

			// 在数组指定索引处追加元素
			template <typename _T_Value, typename... Args>
			bool ArrayInsert(size_t index, _T_Value value, Args... keys);

			// 删除数组指定索引处元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename... Args>
			bool ArrayDelete(size_t index, Args... keys);

			// 在数组指定索引处插入/替换
			template <typename _T_Value, typename... Args>
			bool SetArrayValue(_T_Value value, size_t index, Args... keys);

			// 获取数组长度（路径不存在返回0，不是数组返回-1）
			template <typename... Args>
			int GetArrayLength(Args... keys);

			// 获取数组末尾元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename _T_Value, typename... Args>
#if __ALL_AI_CXX_VERSION >= 17L
			std::optional<_T_Value> GetArrayBack(Args... keys);
#elif __ALL_AI_CXX_VERSION >= 14L
			_T_Value GetArrayBack(Args... keys);
#endif

			// 获取数组头部元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename _T_Value, typename... Args>
#if __ALL_AI_CXX_VERSION >= 17L
			std::optional<_T_Value> GetArrayFront(Args... keys);
#elif __ALL_AI_CXX_VERSION >= 14L
			_T_Value GetArrayFront(Args... keys);
#endif

			// 获取数组指定索引处元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename _T_Value, typename... Args>
#if __ALL_AI_CXX_VERSION >= 17L
			std::optional<_T_Value> GetArrayValue(size_t index, Args... keys);
#elif __ALL_AI_CXX_VERSION >= 14L
			_T_Value GetArrayValue(size_t index, Args... keys);
#endif

			// 创建空数组
			template <typename... Args>
			bool CreateArray(Args... keys);

			// 创建空对象
			template <typename... Args>
			bool CreateObject(Args... keys);

			// 清空数组
			template <typename... Args>
			bool ClearArray(Args... keys);

			// 路径元素类型：可以是字符串键或数组索引
#if __ALL_AI_CXX_VERSION >= 17L
			using PathKey = std::variant<std::string, size_t, int>;
#elif __ALL_AI_CXX_VERSION >= 14L
			struct PathKey {
				enum class Type {
					String,
					SizeT,
					Int
				} type;

				std::string str_val;
				size_t size_val;
				int int_val;

				// 默认构造（vector 某些操作需要）
				PathKey() :
					type(Type::Int),
					str_val(),
					size_val(0),
					int_val(0)
				{
				}

				// 转换构造函数，替代 std::variant 的隐式构造
				PathKey(const std::string& s) :
					type(Type::String),
					str_val(s),
					size_val(0),
					int_val(0)
				{
				}

				PathKey(const char* s) :
					type(Type::String),
					str_val(s),
					size_val(0),
					int_val(0)
				{
				}

				PathKey(size_t v) :
					type(Type::SizeT),
					str_val(),
					size_val(v),
					int_val(0)
				{
				}

				PathKey(int v) :
					type(Type::Int),
					str_val(),
					size_val(0),
					int_val(v)
				{
				}

				static PathKey from_string(const std::string& s)
				{
					return PathKey(s);
				}

				static PathKey from_size_t(size_t v)
				{
					return PathKey(v);
				}

				static PathKey from_int(int v)
				{
					return PathKey(v);
				}
			};
#endif

		private:
			// 终止递归
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path);

			// 字符串键
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const std::string& _key);

			// 数组索引
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const char* _key);

			// 数组索引（size_t 或 int）
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, size_t _index);
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, int _index);

			// 可变参数展开
			template <typename T, typename... Rest>
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... _rest);

			// 将可变参数转换为路径数组
			template <typename... Args>
			std::vector<PathKey> BuildPath(Args&&... _args);

			// 根据路径获取或创建节点（自动创建中间对象/数组）
			nlohmann::json* NavigateOrCreate(nlohmann::json& _root, const std::vector<PathKey>& _path, bool _createMissing = true);

			// 根据路径获取节点（只读，不创建）
			nlohmann::json* Navigate(nlohmann::json& _root, const std::vector<PathKey>& _path);

		private:
			nlohmann::json m_request_json;
			mutable std::mutex m_mutex_request;
		};

		/*
		 ============================================================================
		 Function: SetValue
		 Description: 设置json某个字段指定的值 - 接口
		 Parameters:
			 - _T_Value: 需要设置的值
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 设置成功返回true，否则返回false
		 ============================================================================
		*/
		template <typename _T_Value, typename... Args>
		inline bool JsonRequestBuilder::SetValue(_T_Value value, Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			// 使用新的 NavigateOrCreate 替代原来的递归 _setValue
			std::vector<PathKey> path = BuildPath(keys...);
			if (path.empty())
			{
				return false;
			}

			PathKey lastKey = path.back();
			std::vector<PathKey> parentPath(path.begin(), path.end() - 1);

			nlohmann::json* parent = NavigateOrCreate(m_request_json, parentPath, true);
			if (parent == nullptr)
			{
				return false;
			}

			// 设置值，std::holds_alternative判断变量类型，如果不是string或size_t返回false
			// true - string，键
			// false - size_t，索引
#if __ALL_AI_CXX_VERSION >= 17L
			if (std::holds_alternative<std::string>(lastKey))
			{
				(*parent)[std::get<std::string>(lastKey)] = value;
			}
			else if (std::holds_alternative<size_t>(lastKey))
			{
				size_t index = std::get<size_t>(lastKey);
				if (!parent->is_array() && !parent->is_null())
				{
					return false;
				}
				if (parent->is_null())
				{
					*parent = nlohmann::json::array();
				}
				while (parent->size() <= index)
				{
					parent->push_back(nullptr);
				}
				(*parent)[index] = value;
			}
			else
			{
				return false;
			}
#elif __ALL_AI_CXX_VERSION >= 14L
			switch (lastKey.type)
			{
			case PathKey::Type::String:
				(*parent)[lastKey.str_val] = value;
				break;

			case PathKey::Type::SizeT:
			{
				size_t index = lastKey.size_val;
				if (!parent->is_array() && !parent->is_null())
				{
					return false;
				}
				if (parent->is_null())
				{
					*parent = nlohmann::json::array();
				}
				while (parent->size() <= index)
				{
					parent->push_back(nullptr);
				}
				(*parent)[index] = value;
				break;
			}

			default:
				// Int 类型或其它未知类型，与原逻辑一致返回 false
				return false;
			}
#endif
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayPushBack
		 Description: 在json数组末尾追加元素
		 Parameters:
			 - _T_Value: 需要设置的值
			 - _Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 设置成功返回true，否则返回false
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
		inline bool JsonRequestBuilder::ArrayPushBack(_T_Value value, Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			if (node == nullptr)
			{
				return false;
			}
			if (!node->is_array() && !node->is_null())
			{
				return false;
			}

			if (node->is_null())
			{
				*node = nlohmann::json::array();
			}
			node->push_back(value);
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayDeleteBack
		 Description: 删除json数组末尾的元素
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 如果删除成功返回true，否则返回false
		 ============================================================================
		*/
		template<typename ...Args>
		inline bool JsonRequestBuilder::ArrayDeleteBack(Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			if (node == nullptr || !node->is_array() || node->empty())
			{
				return false;
			}

			node->erase(node->end() - 1);
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayPushFront
		 Description: 在json数组开头追加元素
		 Parameters:
			 - _T_Value: 需要设置的值
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 如果设置成功返回true，否则返回false
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
		inline bool JsonRequestBuilder::ArrayPushFront(_T_Value value, Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			if (node == nullptr)
			{
				return false;
			}
			if (!node->is_array() && !node->is_null())
			{
				return false;
			}

			node->insert(node->begin(), value);
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayDeleteFront
		 Description: 删除json数组开头的元素
		 Parameters:
			 - _T_Value: 需要设置的值
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 如果设置成功返回true，否则返回false
		 ============================================================================
		*/
		template<typename ...Args>
		inline bool JsonRequestBuilder::ArrayDeleteFront(Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			if (node == nullptr || !node->is_array() || node->empty())
			{
				return false;
			}

			node->erase(node->begin());
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayInsert
		 Description: 在json数组指定下标插入元素
		 Parameters:
			 - _T_Value: 需要设置的值
			 - size_t: 下标
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 如果设置成功返回true，否则返回false
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
		inline bool JsonRequestBuilder::ArrayInsert(size_t index, _T_Value value, Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			if (node == nullptr)
			{
				return false;
			}
			if (!node->is_array() && !node->is_null())
			{
				return false;
			}

			node->insert(node->begin() + index, value);
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayDelete
		 Description: 删除json数组指定下标的元素
		 Parameters:
			 - size_t: 下标
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 如果删除成功返回true，否则返回false
		 ============================================================================
		*/
		template<typename ...Args>
		inline bool JsonRequestBuilder::ArrayDelete(size_t index, Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			if (node == nullptr || !node->is_array() || node->size() <= index)
			{
				return false;
			}

			node->erase(node->begin() + index);
			return true;
		}

		/*
		============================================================================
		Function: SetArrayValue
		Description: 设置json数组指定下标的值
		Parameters:
			- _T_Value: 需要设置的值
			- size_t: 下标
			- Args...: 不定参数，必须是string，作为指向json的字段的索引
		Return: 成功返回true，否则返回false
		============================================================================
	   */
		template <typename _T_Value, typename... Args>
		inline bool JsonRequestBuilder::SetArrayValue(_T_Value value, size_t index, Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			if (node == nullptr)
			{
				return false;
			}
			if (!node->is_array() && !node->is_null())
			{
				return false;
			}

			if (node->is_null())
			{
				*node = nlohmann::json::array();
			}

			// 确保索引有效
			if (index > node->size())
			{
				// 扩展数组
				while (node->size() < index)
				{
					node->push_back(nullptr);
				}
			}
			if (index == node->size())
			{
				node->push_back(value);
			}
			else
			{
				(*node)[index] = value;
			}
			return true;
		}

		/*
		 ============================================================================
		 Function: GetArrayLength
		 Description: 获取json数组长度
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 数组长度。参数合法返回数组长度，否则返回-1，节点不存在返回0
		 ============================================================================
		*/
		template <typename... Args>
		inline int JsonRequestBuilder::GetArrayLength(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			// 如果节点不存在，返回0
			if (node == nullptr)
			{
				return 0;
			}

			// 如果不是数组，返回-1
			if (!node->is_array())
			{
				return -1;
			}

			return static_cast<int>(node->size());
		}

		/*
		 ============================================================================
		 Function: GetArrayBack
		 Description: 获取json数组最后一个元素
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 获取成功返回std::optional<_T_Value>，否则返回std::nullopt
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
#if __ALL_AI_CXX_VERSION >= 17L
		inline std::optional<_T_Value> JsonRequestBuilder::GetArrayBack(Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			if (node == nullptr || !node->is_array() || node->empty())
			{
				return std::nullopt;
			}

			return node->back().get<_T_Value>();
		}
#elif __ALL_AI_CXX_VERSION >= 14L
		inline _T_Value JsonRequestBuilder::GetArrayBack(Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			if (node == nullptr || !node->is_array() || node->empty())
			{
				return _T_Value();
			}

			return node->back().get<_T_Value>();
		}
#endif
		

		/*
		 ============================================================================
		 Function: GetArrayFront
		 Description: 获取json数组第一个元素
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 获取成功返回std::optional<_T_Value>，否则返回std::nullopt
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
#if __ALL_AI_CXX_VERSION >= 17L
		inline std::optional<_T_Value> JsonRequestBuilder::GetArrayFront(Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);
			if (node == nullptr || !node->is_array() || node->empty())
			{
				return std::nullopt;
			}
			return node->front().get<_T_Value>();
		}
#elif __ALL_AI_CXX_VERSION >= 14L
		inline _T_Value JsonRequestBuilder::GetArrayFront(Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);
			if (node == nullptr || !node->is_array() || node->empty())
			{
				return _T_Value();
			}
			return node->front().get<_T_Value>();
		}
#endif
		

		/*
		 ============================================================================
		 Function: GetArrayValue
		 Description: 获取json数组指定下标的值
		 Parameters:
			 - size_t: 下标
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 获取成功返回std::optional<_T_Value>，否则返回std::nullopt
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
#if __ALL_AI_CXX_VERSION >= 17L
		inline std::optional<_T_Value> JsonRequestBuilder::GetArrayValue(size_t index, Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);
			if (node == nullptr || !node->is_array() || index >= node->size())
			{
				return std::nullopt;
			}
			return node->at(index).get<_T_Value>();
		}
#elif __ALL_AI_CXX_VERSION >= 14L
		inline _T_Value JsonRequestBuilder::GetArrayValue(size_t index, Args ...keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);
			if (node == nullptr || !node->is_array() || index >= node->size())
			{
				return _T_Value();
			}
			return node->at(index).get<_T_Value>();
		}
#endif
		
		/*
		 ============================================================================
		 Function: CreateArray
		 Description: 创建json数组
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 设置成功返回true，否则返回false
		 ============================================================================
		*/
		template <typename... Args>
		inline bool JsonRequestBuilder::CreateArray(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			// 节点不存在
			if (node == nullptr)
			{
				return false;
			}
			*node = nlohmann::json::array();
			return true;
		}

		/*
		 ============================================================================
		 Function: CreateObject
		 Description: 创建json对象
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 设置成功返回true，否则返回false
		 ============================================================================
		*/
		template <typename... Args>
		inline bool JsonRequestBuilder::CreateObject(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			// 节点不存在
			if (node == nullptr)
			{
				return false;
			}

			*node = nlohmann::json::object();
			return true;
		}

		/*
		 ============================================================================
		 Function: ClearArray
		 Description: 清空json数组
		 Parameters:
			 - Args...: 不定参数，必须是string，作为指向json的字段的索引
		 Return: 设置成功返回true，否则返回false（索引的数组不存在或不是数组）
		 ============================================================================
		*/
		template <typename... Args>
		inline bool JsonRequestBuilder::ClearArray(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			// 如果节点不存在，返回false
			if (node == nullptr)
			{
				return false;
			}

			// 如果不是数组，返回false
			if (node->is_array())
			{
				node->clear();
				return true;
			}

			return false;
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: 构建路径 - 递归终止
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>& path: 路径
		 Return: 无
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path)
		{
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: 构建路径实现
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: 路径
			 - const std::string: 路径
		 Return: 无
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const std::string& _key)
		{
			_path.emplace_back(_key);
			return;
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: 构建路径实现
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: 路径
			 - const char*: 键
		 Return: 无
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const char* _key)
		{
			_path.emplace_back(std::string(_key));
			return;
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: 构建路径实现
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: 路径
			 - size_t: 下标
		 Return: 无
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, size_t _index)
		{
			_path.emplace_back(_index);
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: 构建路径实现
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: 路径
			 - int: 下标
		 Return: 无
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, int _index)
		{
			if (_index < 0)
			{
				throw std::invalid_argument("Array index cannot be negative");
			}
			_path.emplace_back(static_cast<size_t>(_index));
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: 构建路径实现
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: 路径
			 - T&&: 参数 - 首个参数
			 - Rest&&...: 参数 - 剩余参数
		 Return: 无
		 ============================================================================
		*/
		template <typename T, typename... Rest>
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... _rest)
		{
			BuildPathImpl(_path, std::forward<T>(_first));
			BuildPathImpl(_path, std::forward<Rest>(_rest)...);
		}

		/*
		 ============================================================================
		 Function: BuildPath
		 Description: 构建路径
		 Parameters:
			 - Args&&... args: 参数
		 Return: 返回路径
		 ============================================================================
		*/
		template <typename... Args>
		inline std::vector<JsonRequestBuilder::PathKey> JsonRequestBuilder::BuildPath(Args&&... _args)
		{
			std::vector<PathKey> path;
			BuildPathImpl(path, std::forward<Args>(_args)...);
			return path;
		}

		/*
		 ============================================================================
		 Function: NavigateOrCreate
		 Description: 访问json节点并创建
		 Parameters:
			 - nlohmann::json& root: 根节点
			 - const std::vector<PathKey>& path: 路径
			 - bool: 如果路径不存在，是否创建。true - 创建，false - 不创建
		 Return: nlohmann::json*，如果路径不存在，返回nullptr，否则返回节点指针
		 ============================================================================
		*/
		inline nlohmann::json* JsonRequestBuilder::NavigateOrCreate(nlohmann::json& _root, const std::vector<PathKey>& _path, bool _createMissing)
		{
			nlohmann::json* current = &_root;

#if __ALL_AI_CXX_VERSION >= 17L
			for (const PathKey& key : _path)
			{
				std::visit([&](auto&& k) {
					using _Key_T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<_Key_T, std::string>)
					{
						// 当前节点必须是对象；null 在允许创建时可转为对象
						if (!current->is_object())
						{
							if (!_createMissing || !current->is_null())
							{
								current = nullptr;
								return;
							}
							*current = nlohmann::json::object();
						}

						if (!current->contains(k))
						{
							if (!_createMissing)
							{
								current = nullptr;
								return;
							}
							// 填 null 而非 object，让后续索引键有机会将其转为数组
							(*current)[k] = nullptr;
						}
						current = &(*current)[k];
					}
					else if constexpr (std::is_same_v<_Key_T, size_t> || std::is_same_v<_Key_T, int>)
					{
						// int 索引先校验非负，避免隐式转换为巨大 size_t
						if constexpr (std::is_same_v<_Key_T, int>)
						{
							if (k < 0)
							{
								current = nullptr;
								return;
							}
						}
						const size_t idx = static_cast<size_t>(k);

						// 当前节点必须是数组；null 在允许创建时可转为数组
						if (!current->is_array())
						{
							if (!_createMissing || !current->is_null())
							{
								current = nullptr;
								return;
							}
							*current = nlohmann::json::array();
						}

						// 确保数组足够长，不足时用 null 填充
						if (idx >= current->size())
						{
							if (!_createMissing)
							{
								current = nullptr;
								return;
							}
							while (current->size() <= idx)
							{
								current->push_back(nullptr);
							}
						}
						current = &(*current)[idx];
					}
					}, key);

				if (current == nullptr)
				{
					return nullptr;
				}
			}

#elif __ALL_AI_CXX_VERSION >= 14L
			for (const PathKey& key : _path)
			{
				switch (key.type)
				{
				case PathKey::Type::String:
				{
					const std::string& k = key.str_val;

					// 当前节点必须是对象；null 在允许创建时可转为对象
					if (!current->is_object())
					{
						if (!_createMissing || !current->is_null())
						{
							return nullptr;
						}
						*current = nlohmann::json::object();
					}

					if (!current->contains(k))
					{
						if (!_createMissing)
						{
							return nullptr;
						}
						(*current)[k] = nullptr;
					}
					current = &(*current)[k];
					break;
				}

				case PathKey::Type::SizeT:
				case PathKey::Type::Int:
				{
					// int 索引先校验非负，避免隐式转换
					if (key.type == PathKey::Type::Int && key.int_val < 0)
					{
						return nullptr;
					}

					size_t idx = (key.type == PathKey::Type::SizeT) ? key.size_val : static_cast<size_t>(key.int_val);

					// 当前节点必须是数组，null 在允许创建时可转为数组
					if (!current->is_array())
					{
						if (!_createMissing || !current->is_null())
						{
							return nullptr;
						}
						*current = nlohmann::json::array();
					}

					// 确保数组足够长，不足时用 null 填充
					if (idx >= current->size())
					{
						if (!_createMissing)
						{
							return nullptr;
						}
						while (current->size() <= idx)
						{
							current->push_back(nullptr);
						}
					}
					current = &(*current)[idx];
					break;
				}

				default:
					return nullptr;  // 未知类型
				}
			}
#endif
			return current;
		}
		
		/*
		 ============================================================================
		 Function: Navgate
		 Description: 访问json节点
		 Parameters:
			 - nlohmann::json& root: 根节点
			 - const std::vector<PathKey>& path: 路径
		 Return: nlohmann::json*，如果路径不存在，返回nullptr，否则返回节点指针
		 ============================================================================
		*/
		inline nlohmann::json* JsonRequestBuilder::Navigate(nlohmann::json& _root, const std::vector<PathKey>& _path)
		{
			nlohmann::json* current = &_root;
#if __ALL_AI_CXX_VERSION >= 17L
			for (const auto& key : _path)
			{
				// 访问当前节点
				std::visit([&](auto&& k) {
					using _Key_T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<_Key_T, std::string>)
					{
						// 必须是对象且键存在，否则查找失败
						if (!current->is_object() || !current->contains(k))
						{
							current = nullptr;
							return;
						}
						current = &(*current)[k];
					}
					else if constexpr (std::is_same_v<_Key_T, size_t> || std::is_same_v<_Key_T, int>)
					{
						// int 索引先校验非负，避免隐式转换为巨大 size_t
						if constexpr (std::is_same_v<_Key_T, int>)
						{
							if (k < 0)
							{
								current = nullptr;
								return;
							}
						}
						const size_t idx = static_cast<size_t>(k);

						// 必须是数组且索引在范围内，否则查找失败
						if (!current->is_array() || idx >= current->size())
						{
							current = nullptr;
							return;
						}
						current = &(*current)[idx];
					}
					}, key);

				// 如果当前节点为nullptr，返回nullptr
				if (current == nullptr)
				{
					return nullptr;
				}
			}
#elif __ALL_AI_CXX_VERSION >= 14L
			for (const PathKey& key : _path)
			{
				switch (key.type)
				{
				case PathKey::Type::String:
				{
					const std::string& k = key.str_val;

					// 必须是对象且键存在，否则查找失败
					if (!current->is_object() || !current->contains(k))
					{
						return nullptr;
					}
					current = &(*current)[k];
					break;
				}

				case PathKey::Type::SizeT:
				case PathKey::Type::Int:
				{
					// int 索引先校验非负，避免隐式转换为巨大 size_t
					if (key.type == PathKey::Type::Int && key.int_val < 0)
					{
						return nullptr;
					}

					size_t idx = (key.type == PathKey::Type::SizeT)
						? key.size_val
						: static_cast<size_t>(key.int_val);

					// 必须是数组且索引在范围内，否则查找失败
					if (!current->is_array() || idx >= current->size())
					{
						return nullptr;
					}
					current = &(*current)[idx];
					break;
				}

				default:
					return nullptr;  // 未知类型
				}
			}
#endif
			return current;
		}

		// json解析策略
		class JsonResponseParser : public IResponseParserStrategy {
		public:

			JsonResponseParser() {}

			virtual ~JsonResponseParser() override = default;

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
				std::lock_guard<std::mutex> lock(this->m_mutex_response);
				try
				{
					this->m_response_json = std::move(nlohmann::json::parse(response));
				}
				catch (const nlohmann::json::exception& e)
				{
					DoErrorThrow(e.what());
					return;
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
				std::lock_guard<std::mutex> lock(this->m_mutex_response);
				return this->m_response_json;
			}

			// 获取某个字段的值 - 重载
			template <typename _T_Type, typename... _Keys>
			_T_Type GetValue(_Keys... _keys);

		private:

			// 获取json某个字段的值，递归结束层
			template <typename _T_Type>
			_T_Type _getValue(const nlohmann::json& _json);

			// 获取json某个字段的值，递归中间层
			template <typename _T_Type, typename _First, typename... Args>
			_T_Type _getValue(const nlohmann::json& _json, _First&& first, Args... rest);

#if __ALL_AI_CXX_VERSION >= 17L
			// TODO
#elif __ALL_AI_CXX_VERSION >= 14L
			// 获取json某个字段的值，递归中间层 - 数组索引版本
			template <typename _T_Type, typename _Index, typename... Args>
			_T_Type _getValueStep(const nlohmann::json& _json, _Index first, std::true_type, Args... rest);
			// 获取json某个字段的值，递归中间层 - 对象键版本
			template <typename _T_Type, typename _Key, typename... Args>
			_T_Type _getValueStep(const nlohmann::json& _json, _Key&& first, std::false_type, Args... rest);
#endif

		private:
			nlohmann::json m_response_json;
			std::mutex m_mutex_response;
		};

		/*
		 ============================================================================
		 Function: GetValue
		 Description: 获取json某个字段指定的值 - 接口
		 Parameters:
		   - _Keys...: 剩余键（可变参数包），长度可为 0，作为索引
		 Return: 获取成功返回指定类型的值，否则返回与一个空类型
		 ============================================================================
		*/
		template <typename _T_Type, typename... _Keys>
		_T_Type JsonResponseParser::GetValue(_Keys... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_response);
			return _getValue<_T_Type>(this->m_response_json, std::forward<_Keys>(_keys)...);
		}

		/*
		 ============================================================================
		 Function: _getValue
		 Description: 获取json某个字段指定的值 - 接口终止层
		 Parameters:
		   - nlohmann::json&: 待获取值的 json 对象
		 Return: 函数执行成功返回一个特化的值，否则返回空特化值
		 ============================================================================
		*/
		template <typename _T_Type>
		_T_Type JsonResponseParser::_getValue(const nlohmann::json& _json)
		{
			try
			{
				return _json.get<_T_Type>();
			}
			catch (const nlohmann::json::exception& e)
			{
				// 字段存在但类型不匹配（如 content 为 null 却按 string 提取），
				DoErrorThrow(e.what());
				return _T_Type{};
			}
		}

		/*
		 ============================================================================
		 Function: _getValue
		 Description: 设置json某个字段指定的值 - 接口中间层
		   仅当整条路径上的所有中间对象都已存在时才写入，否则放弃并返回 false。
		 Parameters:
		   - nlohmann::json&: 待获取值的 json 对象
		   - _First&&: 路径上的第一个键
		   - Args&&...: 剩余键（可变参数包），长度可为 0
		 Return: 获取成功返回对应的类型的数据，否则返回一个空数据
		 ============================================================================
		*/
#if __ALL_AI_CXX_VERSION >= 17L
		template <typename _T_Type, typename _First, typename... Args>
		_T_Type JsonResponseParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			if constexpr (std::is_integral_v<std::decay_t<_First>>)
			{
				// 数组索引
				if (!_json.is_array() || first < 0 || static_cast<size_t>(first) >= _json.size())
				{
					std::string err = "Array index out of bounds: " + std::to_string(first);
					DoErrorThrow(err);
					return _T_Type{};
				}
				return _getValue<_T_Type>(_json.at(first), std::forward<Args>(rest)...);
			}
			else
			{
				// 对象键
				if (!_json.is_object() || !_json.contains(first))
				{
					std::string err = "Key not found: " + std::string(first);
					DoErrorThrow(err);
					return _T_Type{};
				}
				return _getValue<_T_Type>(_json.at(first), std::forward<Args>(rest)...);
			}
		}
#elif __ALL_AI_CXX_VERSION >= 14L
		template <typename _T_Type, typename _First, typename... Args>
		_T_Type JsonResponseParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			return _getValueStep<_T_Type>(_json, std::forward<_First>(first),
				std::is_integral<std::decay_t<_First>>{},  // 注意：C++14 没有 _v 后缀
				std::forward<Args>(rest)...);
		}

		// 数组索引版本（_First 是整型时选中）
		template <typename _T_Type, typename _Index, typename... Args>
		_T_Type JsonResponseParser::_getValueStep(const nlohmann::json& _json, _Index first, std::true_type, Args... rest)
		{
			if (!_json.is_array() || first < 0 || static_cast<size_t>(first) >= _json.size())
			{
				std::string err = "Array index out of bounds: " + std::to_string(first);
				DoErrorThrow(err);
				return _T_Type{};
			}
			return _getValue<_T_Type>(_json.at(first), std::forward<Args>(rest)...);
		}

		// 对象键版本（_First 非整型时选中）
		template <typename _T_Type, typename _Key, typename... Args>
		_T_Type JsonResponseParser::_getValueStep(const nlohmann::json& _json, _Key&& first, std::false_type, Args... rest)
		{
			if (!_json.is_object() || !_json.contains(first))
			{
				std::string err = "Key not found: " + std::string(first);
				DoErrorThrow(err);
				return _T_Type{};
			}
			return _getValue<_T_Type>(_json.at(first), std::forward<Args>(rest)...);
		}
#endif

	}

	/*
	 ============================================================================
	 Function: JsonGet
	 Description: 无状态JSON取值（自由函数）：直接对传入的json按路径安全取值，
	 无需像JsonResponseParser那样先Parse再GetValue。
	 路径键支持字符串（对象键）与整数（数组下标），与GetValue语义一致；
	 路径不存在或类型不匹配时按错误抛出方式处理并返回 T{}
	 Parameters:
		 - const nlohmann::json& data: 待取值的json对象
		 - Args&&... keys: 路径键（字符串/整数，可变参数）
	 Return: 返回取到的值，失败返回 T{}
	 Example: std::string id = ALL_AI::JsonGet<std::string>(resp, "id");
	          std::string s = ALL_AI::JsonGet<std::string>(resp, "choices", 0, "message", "content");
	 ============================================================================
	*/
	template <typename _T_Type, typename... Args>
	_T_Type JsonGet(const nlohmann::json& data, Args&&... keys)
	{
		JsonOperator::JsonResponseParser parser;
		parser.Parse(data);
		return parser.GetValue<_T_Type>(std::forward<Args>(keys)...);
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
			std::lock_guard<std::mutex> lock(this->m_mutex_json);
			return this->m_array;
		}

		/*
		 ============================================================================
		 Function: PushBack
		 Description: 向消息数组中添加消息
		 Parameters:
			 - const Role&: 消息角色
			 - const std::string&: 消息内容
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

		/*
		 ============================================================================
		 Function: Base64Encode
		 Description: 将二进制数据编码为base64字符串，
					 用于构建视觉模型的base64图片消息（data:image/xxx;base64,...）
		 Parameters:
			 - const std::string& data: 待编码的二进制数据
		 Return: 返回base64编码后的字符串
		 ============================================================================
		*/
		static std::string Base64Encode(const std::string& data)
		{
			static const char base64_table[] =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

			std::string encoded;
			encoded.reserve(((data.size() + 2) / 3) * 4);

			// 每3个字节为一组，编码为4个base64字符，不足3字节的末尾组用'='填充
			for (size_t i = 0; i < data.size(); i += 3)
			{
				unsigned int triple = static_cast<unsigned char>(data[i]) << 16;
				if (i + 1 < data.size())
				{
					triple |= static_cast<unsigned char>(data[i + 1]) << 8;
				}
				if (i + 2 < data.size())
				{
					triple |= static_cast<unsigned char>(data[i + 2]);
				}

				encoded.push_back(base64_table[(triple >> 18) & 0x3F]);
				encoded.push_back(base64_table[(triple >> 12) & 0x3F]);
				encoded.push_back(i + 1 < data.size() ? base64_table[(triple >> 6) & 0x3F] : '=');
				encoded.push_back(i + 2 < data.size() ? base64_table[triple & 0x3F] : '=');
			}

			return encoded;
		}

		/*
		 ============================================================================
		 Function: FileToBase64
		 Description: 读取本地文件（二进制方式）并编码为base64字符串，
					 常用于将本地图片编码后传给视觉模型
		 Parameters:
			 - const std::string& file_path: 本地文件路径
		 Return: 成功返回base64编码后的字符串，文件不存在或不可读返回空字符串
		 ============================================================================
		*/
		static std::string FileToBase64(const std::string& file_path)
		{
			std::ifstream file(file_path, std::ios::binary);
			if (!file.good())
			{
				return "";
			}

			std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			return Base64Encode(data);
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

	// 文件操作相关的类和函数（文件类型识别、多模态内容构建等），
	// 文件处理策略与策略工厂定义在AI类之后（策略依赖AI类的接口）
	namespace FileOperator {

		// 文件类型枚举，决定文件的处理策略
		enum class FileType {
			Unknown,	// 未知类型（默认按文档处理）
			Document,	// 文档/文本类：txt、md、pdf、doc、xls、ppt、csv等
			Image,		// 图片类：jpg、png、gif、webp、bmp、heic等
			Video,		// 视频类：mp4、mov、avi、webm、wmv等
			Audio		// 音频类：mp3、wav、m4a、flac、ogg等
		};

		// 文件用途枚举，对应文件接口的purpose字段
		enum class FilePurpose {
			FileExtract,	// "file-extract"：抽取文件内容（文档/文本类文件）
			Image,			// "image"：上传图片，用于视觉理解
			Video,			// "video"：上传视频，用于视频理解
			Batch			// "batch"：上传JSONL文件，用于批处理任务
		};

		// 图片传入方式枚举
		enum class ImageTransportMode {
			Base64,			// base64编码后直接放入消息（单张图片推荐使用）
			UploadReference	// 上传(purpose=image)后通过文件ID引用（需要多次引用时推荐使用）
		};

		// 文件上传结果
		struct FileUploadResult {
			std::string file_path;							// 本地文件路径
			std::string file_id;							// 上传成功时服务器返回的文件ID
			FileType file_type = FileType::Unknown;			// 识别出的文件类型
			bool success = false;							// 是否上传成功
			nlohmann::json raw_response;					// 服务器原始响应
		};

		/*
		 ============================================================================
		 Class: FileTypeDetector
		 Description: 文件类型识别器，根据文件扩展名识别文件类型、推导默认purpose与MIME类型，
					 全部为静态方法，无需实例化
		 ============================================================================
		*/
		class FileTypeDetector {
		public:

			/*
			 ============================================================================
			 Function: DetectFileType
			 Description: 根据文件扩展名识别文件类型
			 Parameters:
				 - const std::string& file_path: 文件路径
			 Return: 返回识别出的文件类型，无法识别返回FileType::Unknown
			 ============================================================================
			*/
			static FileType DetectFileType(const std::string& file_path)
			{
				static const std::unordered_set<std::string> document_exts = {
					"txt", "md", "pdf", "doc", "docx", "xls", "xlsx",
					"ppt", "pptx", "csv", "json", "xml", "html", "htm", "epub",
					"c", "cpp", "h", "java", "py", "rb", "sql", "js", "ts", "go",
					"hpp", "css", "less", "sass", "scss", "jsonl", "jsonld", "jsonb"
				};
				static const std::unordered_set<std::string> image_exts = {
					"jpg", "jpeg", "png", "gif", "webp", "bmp", "heic", "heif"
				};
				static const std::unordered_set<std::string> video_exts = {
					"mp4", "mpeg", "mov", "avi", "flv", "mpg", "webm", "wmv", "3gpp"
				};
				static const std::unordered_set<std::string> audio_exts = {
					"mp3", "wav", "m4a", "flac", "ogg", "aac", "wma"
				};

				std::string ext = GetExtensionLower(file_path);
				if (image_exts.count(ext) > 0)
				{
					return FileType::Image;
				}
				if (video_exts.count(ext) > 0)
				{
					return FileType::Video;
				}
				if (audio_exts.count(ext) > 0)
				{
					return FileType::Audio;
				}
				if (document_exts.count(ext) > 0)
				{
					return FileType::Document;
				}
				return FileType::Unknown;
			}

			/*
			 ============================================================================
			 Function: GetDefaultPurpose
			 Description: 获取文件类型对应的默认purpose
			 Parameters:
				 - FileType file_type: 文件类型
			 Return: 返回默认的文件用途
			 ============================================================================
			*/
			static FilePurpose GetDefaultPurpose(FileType file_type)
			{
				switch (file_type)
				{
				case FileType::Image:
					return FilePurpose::Image;
				case FileType::Video:
					return FilePurpose::Video;
				case FileType::Document:
				case FileType::Audio:
				case FileType::Unknown:
				default:
					// 文档与未知类型默认抽取内容；音频默认按file-extract处理（部分平台支持音频转写），
					// 如需其他处理方式可通过FileStrategyFactory::RegisterStrategy注册自定义策略
					return FilePurpose::FileExtract;
				}
			}

			/*
			 ============================================================================
			 Function: PurposeToString
			 Description: 将文件用途枚举转换为API的purpose字符串
			 Parameters:
				 - FilePurpose purpose: 文件用途
			 Return: 返回purpose字符串
			 ============================================================================
			*/
			static std::string PurposeToString(FilePurpose purpose)
			{
				switch (purpose)
				{
				case FilePurpose::FileExtract:
					return "file-extract";
				case FilePurpose::Image:
					return "image";
				case FilePurpose::Video:
					return "video";
				case FilePurpose::Batch:
					return "batch";
				default:
					return "file-extract";
				}
			}

			/*
			 ============================================================================
			 Function: GetMimeType
			 Description: 根据文件扩展名获取MIME类型（构建base64 data URL时使用）
			 Parameters:
				 - const std::string& file_path: 文件路径
			 Return: 返回MIME类型字符串，无法识别返回"application/octet-stream"
			 ============================================================================
			*/
			static std::string GetMimeType(const std::string& file_path)
			{
				static const std::unordered_map<std::string, std::string> mime_map = {
					// document
					{"txt", "text/plain"},
					// Image
					{"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"png", "image/png"},
					{"gif", "image/gif"}, {"webp", "image/webp"}, {"bmp", "image/bmp"},
					{"heic", "image/heic"}, {"heif", "image/heif"},
					// Video
					{"mp4", "video/mp4"}, {"mpeg", "video/mpeg"}, {"mov", "video/quicktime"},
					{"avi", "video/x-msvideo"}, {"flv", "video/x-flv"}, {"mpg", "video/mpeg"},
					{"webm", "video/webm"}, {"wmv", "video/x-ms-wmv"}, {"3gpp", "video/3gpp"},
					// Audio
					{"mp3", "audio/mpeg"}, {"wav", "audio/wav"}, {"m4a", "audio/mp4"},
					{"flac", "audio/flac"}, {"ogg", "audio/ogg"}, {"aac", "audio/aac"},
					{"wma", "audio/x-ms-wma"}, 
					// other
					{"pdf", "application/pdf"}
				};

				std::string ext = GetExtensionLower(file_path);
				auto it = mime_map.find(ext);
				if (it != mime_map.end())
				{
					return it->second;
				}
				return "application/octet-stream";
			}

		private:

			/*
			 ============================================================================
			 Function: GetExtensionLower
			 Description: 提取文件扩展名并转换为小写（内部辅助函数）
			 Parameters:
				 - const std::string& file_path: 文件路径
			 Return: 返回小写扩展名（不含点号），无扩展名返回空字符串
			 ============================================================================
			*/
			static std::string GetExtensionLower(const std::string& file_path)
			{
				size_t dot_pos = file_path.find_last_of('.');
				size_t sep_pos = file_path.find_last_of("/\\");

				// 点号不存在，或点号在路径分隔符之前（属于目录名而非扩展名）
				if (dot_pos == std::string::npos ||
					(sep_pos != std::string::npos && dot_pos < sep_pos))
				{
					return "";
				}

				std::string ext = file_path.substr(dot_pos + 1);
				for (char& c : ext)
				{
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}
				return ext;
			}
		};

		/*
		 ============================================================================
		 Class: ContentPartBuilder
		 Description: 多模态内容part构建器（Builder模式），以链式调用构建视觉模型的content parts，
					 例如：ContentPartBuilder().AddText("描述图片").AddImageBase64("a.jpg").BuildUserMessage()
		 ============================================================================
		*/
		class ContentPartBuilder {
		public:

			/*
			 ============================================================================
			 Function: ContentPartBuilder
			 Description: 构造函数
			 Parameters:
				 - 无参数: 无释义
			 Return: 无
			 ============================================================================
			*/
			ContentPartBuilder()
				: m_parts(nlohmann::json::array())
			{
			}

			/*
			 ============================================================================
			 Function: AddText
			 Description: 添加文本part
			 Parameters:
				 - const std::string& text: 文本内容
			 Return: 返回构建器自身引用，支持链式调用
			 ============================================================================
			*/
			ContentPartBuilder& AddText(const std::string& text)
			{
				m_parts.push_back({ {"type", "text"}, {"text", text} });
				return *this;
			}

			/*
			 ============================================================================
			 Function: AddImageBase64
			 Description: 添加本地图片part（读取文件并base64编码为data URL）
			 Parameters:
				 - const std::string& file_path: 本地图片路径
			 Return: 返回构建器自身引用，支持链式调用。文件读取失败时不添加part
			 ============================================================================
			*/
			ContentPartBuilder& AddImageBase64(const std::string& file_path)
			{
				std::string base64_data = JsonOperatorTools::FileToBase64(file_path);
				if (base64_data.empty())
				{
					return *this;
				}

				std::string mime = FileTypeDetector::GetMimeType(file_path);
				m_parts.push_back({
					{"type", "image_url"},
					{"image_url", {{"url", "data:" + mime + ";base64," + base64_data}}}
					});
				return *this;
			}

			/*
			 ============================================================================
			 Function: AddImageFileId
			 Description: 添加已上传图片的part（通过文件ID引用，需先以purpose="image"上传）
			 Parameters:
				 - const std::string& file_id: 文件ID
			 Return: 返回构建器自身引用，支持链式调用
			 ============================================================================
			*/
			ContentPartBuilder& AddImageFileId(const std::string& file_id)
			{
				m_parts.push_back({
					{"type", "image_url"},
					{"image_url", {{"url", "ms://" + file_id}}}
					});
				return *this;
			}

			/*
			 ============================================================================
			 Function: AddVideoFileId
			 Description: 添加已上传视频的part（通过文件ID引用，需先以purpose="video"上传）
			 Parameters:
				 - const std::string& file_id: 文件ID
			 Return: 返回构建器自身引用，支持链式调用
			 ============================================================================
			*/
			ContentPartBuilder& AddVideoFileId(const std::string& file_id)
			{
				m_parts.push_back({
					{"type", "video_url"},
					{"video_url", {{"url", "ms://" + file_id}}}
					});
				return *this;
			}

			/*
			 ============================================================================
			 Function: AddAudioBase64
			 Description: 添加本地音频part（读取文件并base64编码为data URL，
			 OpenAI兼容的audio_url内容part），用于音频理解模型的音频输入
			 Parameters:
				 - const std::string& file_path: 本地音频路径
			 Return: 返回构建器自身引用，支持链式调用。文件读取失败时不添加part
			 ============================================================================
			*/
			ContentPartBuilder& AddAudioBase64(const std::string& file_path)
			{
				std::string base64_data = JsonOperatorTools::FileToBase64(file_path);
				if (base64_data.empty())
				{
					return *this;
				}

				std::string mime = FileTypeDetector::GetMimeType(file_path);
				m_parts.push_back({
					{"type", "audio_url"},
					{"audio_url", {{"url", "data:" + mime + ";base64," + base64_data}}}
					});
				return *this;
			}

			/*
			 ============================================================================
			 Function: BuildParts
			 Description: 构建content parts数组
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回content parts数组
			 ============================================================================
			*/
			nlohmann::json BuildParts() const
			{
				return m_parts;
			}

			/*
			 ============================================================================
			 Function: BuildUserMessage
			 Description: 构建一条完整的user消息（content为parts数组）
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回user消息json
			 ============================================================================
			*/
			nlohmann::json BuildUserMessage() const
			{
				return { {"role", "user"}, {"content", m_parts} };
			}

		private:
			nlohmann::json m_parts;	// content parts数组
		};
	}

	// 抽象HTTP传输接口，定义了发送HTTP请求的方法
	class IHttpTransport : public ThrowError {
	public:
		IHttpTransport() = default;
		virtual ~IHttpTransport() = default;
		// 初始化HTTP传输接口，设置URL、API Key和错误抛出方式
		virtual bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
		// 发送HTTP请求
		virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;
		// 清除HTTP传输接口的资源
		virtual void ClearResource() = 0;

		/*
		 ============================================================================
		 Function: SendMultipartRequest
		 Description: 发送multipart/form-data表单请求（文件上传），默认实现为不支持，
					 由具体的传输实现类覆盖。该接口为虚函数而非纯虚函数，
					 以保证用户已实现的自定义传输类无需修改即可继续编译
		 Parameters:
			 - const std::string& url: 文件接口的完整URL（例如 https://api.moonshot.cn/v1/files）
			 - const std::string& file_path: 本地文件路径
			 - const std::string& file_field_name: 表单中文件字段的名称（OpenAI兼容接口为"file"）
			 - const std::unordered_map<std::string, std::string>& form_fields: 除文件外的其他表单字段（例如 purpose）
		 Return: 返回一个nlohmann::json，表示服务器的回复内容
		 ============================================================================
		*/
		virtual nlohmann::json SendMultipartRequest(const std::string& url,
			const std::string& file_path,
			const std::string& file_field_name,
			const std::unordered_map<std::string, std::string>& form_fields)
		{
			DoErrorThrow("IHttpTransport: SendMultipartRequest is not supported by this transport");
			return nlohmann::json{};
		}

		/*
		 ============================================================================
		 Function: SendRequestRaw
		 Description: 发送普通HTTP请求并返回原始响应字符串（不做JSON解析），
					 用于获取文件内容等不一定是JSON的响应。默认实现为不支持，
					 由具体的传输实现类覆盖
		 Parameters:
			 - HttpMethod method: HTTP请求方法
			 - const std::string& url: 请求的完整URL
		 Return: 返回原始响应字符串，失败返回空字符串
		 ============================================================================
		*/
		virtual std::string SendRequestRaw(HttpMethod method, const std::string& url)
		{
			DoErrorThrow("IHttpTransport: SendRequestRaw is not supported by this transport");
			return std::string{};
		}

		/*
		 ============================================================================
		 Function: SendRequestRaw
		 Description: 发送普通HTTP请求（可携带JSON请求体），并通过数据回调逐块交付响应，
		 用于返回音频二进制流等非JSON响应的接口（如TTS）。
		 默认实现忽略body与回调，回退到双参数重载，
		 以保证用户已实现的自定义传输类无需修改即可继续编译
		 Parameters:
			 - HttpMethod method: HTTP请求方法
			 - const std::string& url: 请求的完整URL
			 - const nlohmann::json* body: 可选的JSON请求体，nullptr表示不携带
			 - DataCallback data_callback: 数据回调，空回调表示收完响应后整体返回
		 Return: 返回原始响应字符串（设置了回调时通常为空，数据归回调处理），失败返回空字符串
		 ============================================================================
		*/
		virtual std::string SendRequestRaw(HttpMethod method, const std::string& url,
			const nlohmann::json* body, DataCallback data_callback)
		{
			(void)body;
			(void)data_callback;
			return SendRequestRaw(method, url);
		}
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
				ClearResource();
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
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				if (url.empty() || api_key.empty())
				{
					DoErrorThrow("CurlHttpTransport: url or api_key is empty");
					return false;
				}
				this->m_url = url;
				this->m_key = api_key;
				this->m_error_throw_method = all_ai_error_throw;

				// 判断是否已经初始化过
				// 如果已经初始化过，则关闭已经初始化的libcurl
				if (this->m_curl != nullptr)
				{
					ClearResource();
				}

				// 初始化 libcurl
				this->m_curl = curl_easy_init();
				if (!this->m_curl)
				{
					// 如果初始化失败，根据错误抛出方式处理错误
					DoErrorThrow("CurlHttpTransport: curl_easy_init failed");
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
			   - HttpMethod: HTTP请求方法
			   - const nlohmann::json: 一个指向nlohmann::json对象的指针，表示请求的JSON数据
			 Return: 返回一个nlohmann::json，表示回复内容
			 ============================================================================
			*/
			virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// 如果初始化失败，则在请求时返回空json
				if (this->m_curl == nullptr)
				{
					DoErrorThrow("CurlHttpTransport: curl is not initialized or failed to initialize");
					return nlohmann::json{};
				}
				if (method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "POST");
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "GET");
				}
				else if (method == HttpMethod::DELETE)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
				}
				else
				{
					return nlohmann::json{};
				}

				struct curl_slist* headers = nullptr;
				const bool is_stream = request_json.contains("stream") && request_json["stream"].is_boolean() && request_json["stream"].get<bool>();
				headers = curl_slist_append(headers, is_stream ? "Accept: text/event-stream" : "Accept: application/json");
				if (this->m_key.empty())
				{
					DoErrorThrow("CurlHttpTransport: API key is empty");
					return nlohmann::json{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());
				headers = curl_slist_append(headers, "Content-Type: application/json");
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// 清理可能的残留标志
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// 恢复默认的请求体长度（-1 = 按strlen计算）：TTS等请求设置过显式POSTFIELDSIZE，
				// 不重置会导致后续请求的请求体被按旧长度截断（服务器收到残缺的JSON而报400）
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 及以上：清理可能的multipart残留标志
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
#endif
				// 确保URL为初始化时的URL（文件相关请求会临时切换URL，这里做一次兜底恢复）
				curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());

				// 设置请求数据
				// 使用error_handler_t::replace而非默认的strict：
				// 用户字符串中混入非法UTF-8字节时（常见于MSVC下源文件被保存为GBK编码，
				// 中文字符串字面量变成GBK字节），序列化会将其替换为U+FFFD而不是抛出type_error.316异常
				std::string str_json = request_json.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
				if (method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, str_json.c_str());
					// 显式指定请求体长度，避免依赖默认strlen的同时与清理逻辑保持一致
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(str_json.size()));
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
					curl_slist_free_all(headers); // Ensure we free headers 确保释放headers
					DoErrorThrow(error_message);
					return nlohmann::json{};
				}

				// Check HTTP response code
				long http_code = 0;
				curl_easy_getinfo(this->m_curl, CURLINFO_RESPONSE_CODE, &http_code);
				if (http_code < 200 || http_code >= 300)
				{
					std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
					curl_slist_free_all(headers);
					DoErrorThrow(error_message);
					return nlohmann::json{};
				}

				// Check if response is empty
				if (str_Buffer.empty())
				{
					curl_slist_free_all(headers);
					DoErrorThrow("Empty response received from server");
					return nlohmann::json{};
				}

				// 如果解析失败，就尝试解析 SSE 响应
				// 如果POST请求的stream字段为true
				// 那么try中使用nlohmann::json::parse函数进行解析必定失败
				// 故需要尝试解析SSE响应
				nlohmann::json json_result;
				try
				{
					json_result = nlohmann::json::parse(str_Buffer);
				}
				catch (const nlohmann::json::parse_error& e)
				{
					if (!TryParseSseResponse(str_Buffer, json_result))
					{
						std::string error_message = "Error: Session: JSON parse failed. Response: " + str_Buffer + ", Error: " + e.what();
						curl_slist_free_all(headers);
						DoErrorThrow(error_message);
						return nlohmann::json{};
					}
				}

				// Clean up headers
				curl_slist_free_all(headers);
				return json_result;
			}

			/*
			 ============================================================================
			 Function: SendMultipartRequest
			 Description: 发送multipart/form-data表单请求（文件上传），
						 使用libcurl的mime接口构建表单，兼容OpenAI格式的 /v1/files 文件上传接口
			 Parameters:
				 - const std::string& url: 文件接口的完整URL（例如 https://api.moonshot.cn/v1/files）
				 - const std::string& file_path: 本地文件路径
				 - const std::string& file_field_name: 表单中文件字段的名称（OpenAI兼容接口为"file"）
				 - const std::unordered_map<std::string, std::string>& form_fields: 除文件外的其他表单字段（例如 purpose）
			 Return: 返回一个nlohmann::json，表示服务器的回复内容。如果请求失败，返回一个空的nlohmann::json对象
			 ============================================================================
			*/
			virtual nlohmann::json SendMultipartRequest(const std::string& url,
				const std::string& file_path,
				const std::string& file_field_name,
				const std::unordered_map<std::string, std::string>& form_fields) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// 如果初始化失败，则在请求时返回空json
				if (this->m_curl == nullptr)
				{
					DoErrorThrow("CurlHttpTransport: curl is not initialized or failed to initialize");
					return nlohmann::json{};
				}

#if LIBCURL_VERSION_NUM < 0x073800	// libcurl 7.56.0 以下不支持mime接口
				DoErrorThrow("CurlHttpTransport: SendMultipartRequest requires libcurl 7.56.0 or later");
				return nlohmann::json{};
#else
				// 检查本地文件是否存在且可读（file_path为空时跳过，表示纯字段multipart）
				if (!file_path.empty())
				{
					std::ifstream file_check(file_path, std::ios::binary);
					if (!file_check.good())
					{
						DoErrorThrow("CurlHttpTransport: cannot open file: " + file_path);
						return nlohmann::json{};
					}
				}

				// 构建multipart表单
				curl_mime* mime = curl_mime_init(this->m_curl);
				if (mime == nullptr)
				{
					DoErrorThrow("CurlHttpTransport: curl_mime_init failed");
					return nlohmann::json{};
				}

				// 添加文件字段，libcurl会自动读取文件内容并填充文件名（file_path为空时跳过）
				curl_mimepart* part = nullptr;
				if (!file_path.empty())
				{
					part = curl_mime_addpart(mime);
					curl_mime_name(part, file_field_name.c_str());
					curl_mime_filedata(part, file_path.c_str());
				}

				// 添加其他普通表单字段（例如 purpose=file-extract）
				for (const auto& field : form_fields)
				{
					part = curl_mime_addpart(mime);
					curl_mime_name(part, field.first.c_str());
					curl_mime_data(part, field.second.c_str(), CURL_ZERO_TERMINATED);
				}

				// 设置请求头，multipart的Content-Type由libcurl自动生成（含boundary），切勿手动设置
				struct curl_slist* headers = nullptr;
				if (this->m_key.empty())
				{
					curl_mime_free(mime);
					DoErrorThrow("CurlHttpTransport: API key is empty");
					return nlohmann::json{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// 清理可能的残留标志，避免上一次请求的状态污染本次请求
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// 恢复默认的请求体长度（-1 = 按strlen计算），清除之前请求遗留的显式POSTFIELDSIZE
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 0L);

				// 设置文件接口URL与multipart表单
				curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, mime);

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// 执行请求
				CURLcode res = curl_easy_perform(this->m_curl);

				// 恢复URL与表单状态，避免影响后续的普通JSON请求
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());
				curl_mime_free(mime);
				curl_slist_free_all(headers);

				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					DoErrorThrow(error_message);
					return nlohmann::json{};
				}

				// Check HTTP response code
				long http_code = 0;
				curl_easy_getinfo(this->m_curl, CURLINFO_RESPONSE_CODE, &http_code);
				if (http_code < 200 || http_code >= 300)
				{
					std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
					DoErrorThrow(error_message);
					return nlohmann::json{};
				}

				// Check if response is empty
				if (str_Buffer.empty())
				{
					DoErrorThrow("Empty response received from server");
					return nlohmann::json{};
				}

				// 解析响应JSON
				nlohmann::json json_result;
				try
				{
					json_result = nlohmann::json::parse(str_Buffer);
				}
				catch (const nlohmann::json::parse_error& e)
				{
					std::string error_message = "Error: Session: JSON parse failed. Response: " + str_Buffer + ", Error: " + e.what();
					DoErrorThrow(error_message);
					return nlohmann::json{};
				}

				return json_result;
#endif
			}

			/*
			 ============================================================================
			 Function: SendRequestRaw
			 Description: 发送普通HTTP请求并返回原始响应字符串（不做JSON解析），
						 用于获取文件内容、文件列表等接口。请求结束后会恢复初始化时的URL
			 Parameters:
				 - HttpMethod method: HTTP请求方法
				 - const std::string& url: 请求的完整URL
			 Return: 返回原始响应字符串，失败返回空字符串
			 ============================================================================
			*/
			virtual std::string SendRequestRaw(HttpMethod method, const std::string& url) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// 如果初始化失败，则在请求时返回空字符串
				if (this->m_curl == nullptr)
				{
					DoErrorThrow("CurlHttpTransport: curl is not initialized or failed to initialize");
					return std::string{};
				}

				if (method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "POST");
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "GET");
				}
				else if (method == HttpMethod::DELETE)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
				}
				else
				{
					return std::string{};
				}

				// 设置请求头
				struct curl_slist* headers = nullptr;
				if (this->m_key.empty())
				{
					DoErrorThrow("CurlHttpTransport: API key is empty");
					return std::string{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// 清理可能的残留标志
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// 恢复默认的请求体长度（-1 = 按strlen计算）：TTS等请求设置过显式POSTFIELDSIZE，
				// 不重置会导致后续请求的请求体被按旧长度截断（服务器收到残缺的JSON而报400）
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 及以上：清理可能的multipart残留标志
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
#endif
				if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				// 设置目标URL
				curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// 执行请求
				CURLcode res = curl_easy_perform(this->m_curl);

				// 恢复URL，避免影响后续的普通JSON请求
				curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());
				curl_slist_free_all(headers);

				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					DoErrorThrow(error_message);
					return std::string{};
				}

				// Check HTTP response code
				long http_code = 0;
				curl_easy_getinfo(this->m_curl, CURLINFO_RESPONSE_CODE, &http_code);
				if (http_code < 200 || http_code >= 300)
				{
					std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
					DoErrorThrow(error_message);
					return std::string{};
				}

				return str_Buffer;
			}

			/*
			 ============================================================================
			 Function: SendRequestRaw
			 Description: 发送普通HTTP请求（可携带JSON请求体），并通过数据回调逐块交付响应，
			 设置了回调时响应不再收集到返回字符串中（数据归回调处理），
			 否则行为与双参数重载一致
			 Parameters:
				 - HttpMethod method: HTTP请求方法
				 - const std::string& url: 请求的完整URL
				 - const nlohmann::json* body: 可选的JSON请求体，nullptr表示不携带
				 - DataCallback data_callback: 数据回调，空回调表示收完响应后整体返回
			 Return: 返回原始响应字符串（设置了回调时为空字符串），失败返回空字符串
			 ============================================================================
			*/
			virtual std::string SendRequestRaw(HttpMethod method, const std::string& url,
				const nlohmann::json* body, DataCallback data_callback) override
			{
				// 未携带body且未设置回调时，直接走双参数重载
				if (body == nullptr && !data_callback)
				{
					return SendRequestRaw(method, url);
				}

				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// 如果初始化失败，则在请求时返回空字符串
				if (this->m_curl == nullptr)
				{
					DoErrorThrow("CurlHttpTransport: curl is not initialized or failed to initialize");
					return std::string{};
				}

				if (method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "POST");
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "GET");
				}
				else if (method == HttpMethod::DELETE)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, "DELETE");
				}
				else
				{
					return std::string{};
				}

				// 设置请求头
				struct curl_slist* headers = nullptr;
				if (this->m_key.empty())
				{
					DoErrorThrow("CurlHttpTransport: API key is empty");
					return std::string{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());

				// 清理可能的残留标志
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// 恢复默认的请求体长度（-1 = 按strlen计算）：TTS等请求设置过显式POSTFIELDSIZE，
				// 不重置会导致后续请求的请求体被按旧长度截断（服务器收到残缺的JSON而报400）
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 及以上：清理可能的multipart残留标志
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
#endif
				if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				// 携带JSON请求体（TTS等接口需要）
				std::string str_body;
				if (body != nullptr)
				{
					str_body = body->dump();
					headers = curl_slist_append(headers, "Content-Type: application/json");
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, str_body.c_str());
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(str_body.size()));
				}
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// 设置目标URL
				curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());

				// 设置了数据回调：响应逐块交给用户（不再收集），否则收完整体返回
				std::string str_Buffer;
				if (data_callback)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, DataCallbackWriter);
					curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &data_callback);
				}
				else
				{
					curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
					curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);
				}

				// 执行请求
				CURLcode res = curl_easy_perform(this->m_curl);

				// 恢复URL，避免影响后续的普通JSON请求
				curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());
				curl_slist_free_all(headers);

				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					DoErrorThrow(error_message);
					return std::string{};
				}

				// Check HTTP response code
				// 注意：设置了回调时str_Buffer为空，错误信息中不含响应体（响应已交给回调）
				long http_code = 0;
				curl_easy_getinfo(this->m_curl, CURLINFO_RESPONSE_CODE, &http_code);
				if (http_code < 200 || http_code >= 300)
				{
					std::string error_message = "HTTP error: " + std::to_string(http_code) + ", Response: " + str_Buffer;
					DoErrorThrow(error_message);
					return std::string{};
				}

				return str_Buffer;
			}

			/*
			 ============================================================================
			 Function: ClearResource
			 Description: 清除HTTP传输接口
			 Parameters:
				- 无参数: 无释义
			 Return: 无返回值
			 ============================================================================
			*/
			virtual void ClearResource() override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);
				if (this->m_curl != nullptr)
				{
					// 清理libcurl
					curl_easy_cleanup(this->m_curl);
					this->m_curl = nullptr;
				}
				return;
			}
		private:

			/*
			 ============================================================================
			 Function: Trim
			 Description: 去除字符串首尾的空白字符
			 Parameters:
			   - const std::string& input: 输入字符串
			 Return: 去除首尾空白字符后的字符串
			 ============================================================================
			*/
			static std::string Trim(const std::string& input)
			{
				size_t begin = 0;
				while (begin < input.size() &&
					(input[begin] == ' ' || input[begin] == '\t' || input[begin] == '\r' || input[begin] == '\n'))
				{
					++begin;
				}

				size_t end = input.size();
				while (end > begin &&
					(input[end - 1] == ' ' || input[end - 1] == '\t' || input[end - 1] == '\r' || input[end - 1] == '\n'))
				{
					--end;
				}

				return input.substr(begin, end - begin);
			}

			/*
			 ============================================================================
			 Function: TryParseSseResponse
			 Description: 尝试解析 SSE 响应
			 Parameters:
			   - const std::string& response: 响应字符串
			   - nlohmann::json& json_result: 解析后的 JSON 对象
			 Return: bool: 是否成功解析
			 ============================================================================
			*/
			static bool TryParseSseResponse(const std::string& response, nlohmann::json& json_result)
			{
				nlohmann::json chunks = nlohmann::json::array();

				// SSE 响应通常以"data:"开头，并以"\n\n"结尾，表示一个事件的结束。
				// 要逐行解析响应，提取以"data: "开头的行，并将其内容作为 JSON 进行解析。
				size_t start = 0;
				while (start <= response.size())
				{
					size_t end = response.find('\n', start);
					std::string line;
					if (end == std::string::npos)
					{
						line = response.substr(start);
						start = response.size() + 1;
					}
					else
					{
						line = response.substr(start, end - start);
						start = end + 1;
					}

					// 去除行首尾空白
					line = Trim(line);
					if (line.empty() || line.rfind(":", 0) == 0)
					{
						continue;
					}

					// rfind的原因是因为防止可能存在多个data:
					if (line.rfind("data:", 0) != 0)
					{
						continue;
					}

					// 去除"data: "
					std::string payload = Trim(line.substr(5));
					if (payload.empty())
					{
						continue;
					}

					if (payload == "[DONE]")
					{
						break;
					}

					try
					{
						chunks.push_back(nlohmann::json::parse(payload));
					}
					catch (const nlohmann::json::parse_error&)
					{
						continue;
					}
				}

				// 如果 chunks 为空，说明解析失败，返回 false
				if (chunks.empty())
				{
					return false;
				}

				json_result = chunks.back();
				json_result["sse_chunks"] = chunks;

				nlohmann::json merged_choices = nlohmann::json::array();
				std::unordered_map<int, size_t> choice_index_to_pos;

				// 确保每个 choice index 都有一个对应的 merged_choice 对象，如果没有就创建一个新的
				auto ensure_choice = [&](int index) -> nlohmann::json&
					{
						auto it = choice_index_to_pos.find(index);
						if (it == choice_index_to_pos.end())
						{
							size_t pos = merged_choices.size();
							merged_choices.push_back({
								{"index", index},
								{"message", {{"role", "assistant"}, {"content", ""}}},
								{"finish_reason", nullptr}
								});
							choice_index_to_pos[index] = pos;
							return merged_choices[pos];
						}
						return merged_choices[it->second];
					};

				// 合并 chunks 中的 choices
				for (const nlohmann::json& chunk : chunks)
				{
					if (!chunk.is_object() || !chunk.contains("choices") || !chunk["choices"].is_array())
					{
						continue;
					}

					// 合并 chunk 中的 choices
					for (const nlohmann::json& choice : chunk["choices"])
					{
						// 安全提取index，避免index字段类型异常时value()抛出type_error
						int index = 0;
						if (choice.is_object() && choice.contains("index") && choice["index"].is_number_integer())
						{
							index = choice["index"].get<int>();
						}
						nlohmann::json& merged_choice = ensure_choice(index);

						// 合并 delta
						if (choice.contains("delta") && choice["delta"].is_object())
						{
							const auto& delta = choice["delta"];
							if (delta.contains("role") && delta["role"].is_string())
							{
								merged_choice["message"]["role"] = delta["role"];
							}
							if (delta.contains("content") && delta["content"].is_string())
							{
								merged_choice["message"]["content"] =
									merged_choice["message"]["content"].get<std::string>() + delta["content"].get<std::string>();
							}
						}

						// 合并 text
						if (choice.contains("text") && choice["text"].is_string())
						{
							merged_choice["message"]["content"] =
								merged_choice["message"]["content"].get<std::string>() + choice["text"].get<std::string>();
						}

						// 合并 finish_reason
						if (choice.contains("finish_reason"))
						{
							merged_choice["finish_reason"] = choice["finish_reason"];
						}
					}
				}

				// 如果merged_choices不为空，将其赋值给json_result的choices字段
				if (!merged_choices.empty())
				{
					json_result["choices"] = merged_choices;
				}

				return true;
			}

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

			/*
			 ============================================================================
			 Function: DataCallbackWriter
			 Description: libcurl写数据回调函数（数据回调模式），将收到的数据块转发给用户回调，
			 数据块不做存储；用户回调的返回值透传给libcurl（不等则中止传输）
			 Parameters:
			   - contents: 指向接收到的数据缓冲区
			   - size: 每个数据块的字节大小
			   - nmemb: 数据块的个数
			   - userp: 用户自定义指针，此处指向DataCallback对象
			 Return: 返回用户回调消费的字节数。若与size*nmemb不符，libcurl判定为错误并中止传输
			 ============================================================================
			*/
			static size_t DataCallbackWriter(void* contents, size_t size, size_t nmemb, DataCallback* userp)
			{
				size_t total_size = size * nmemb;
				return (*userp)(static_cast<const char*>(contents), total_size);
			}

		private:

			CURL* m_curl = nullptr;		// libcurl句柄
			std::mutex m_mutex_curl_request;

			std::string m_url;	// API - URL
			std::string m_key;		// API - Key
		};
	}

	// AI类前向声明（FileGateway持有AI反向引用）
	class AI;

	/*
	 ============================================================================
	 Class: FileGateway
	 Description: 文件网关（领域子对象，AI类的公有值成员 ai.Files），
	 封装OpenAI兼容的 /v1/files REST资源结构：上传/批量上传/文件转对话/
	 列表/详情/内容/删除。网关自身零状态——目标URL一律由调用方显式传入
	 （URL是用户的资产，库不存储、不推导、不映射任何业务端点）
	 ============================================================================
	*/
	class FileGateway : public ThrowError {
	public:

		/*
		 ============================================================================
		 Function: FileGateway
		 Description: 构造函数（仅由AI类构造注入，不提供无参构造）
		 Parameters:
			 - AI& ai: 宿主AI对象引用（反向引用，仅用于调用发送能力）
		 Return: 无
		 ============================================================================
		*/
		explicit FileGateway(AI& ai) : m_ai(ai) {}
		~FileGateway() = default;
		FileGateway(const FileGateway&) = delete;
		FileGateway& operator=(const FileGateway&) = delete;

		/*
		 ============================================================================
		 Function: Upload
		 Description: 上传文件到文件接口（multipart/form-data表单），
		 KIMI（Moonshot）等站点的文件接口与OpenAI格式一致，purpose一般为"file-extract"
		 Parameters:
			 - const std::string& file_path: 本地文件路径
			 - const std::string& purpose: 文件用途，KIMI为"file-extract"，OpenAI为"assistants"/"fine-tune"等
			 - const std::string& url: 文件接口的完整URL（必填，例如 https://api.moonshot.cn/v1/files）
		 Return: 返回服务器回复的json（通常包含文件id），失败返回空json对象
		 ============================================================================
		*/
		nlohmann::json Upload(const std::string& file_path,
			const std::string& purpose,
			const std::string& url);

		/*
		 ============================================================================
		 Function: Upload
		 Description: 上传文件到文件接口（FilePurpose枚举重载版本）
		 Parameters:
			 - const std::string& file_path: 本地文件路径
			 - FileOperator::FilePurpose purpose: 文件用途枚举
			 - const std::string& url: 文件接口的完整URL（必填）
		 Return: 返回服务器回复的json，失败返回空json对象
		 ============================================================================
		*/
		nlohmann::json Upload(const std::string& file_path,
			FileOperator::FilePurpose purpose,
			const std::string& url);

		/*
		 ============================================================================
		 Function: UploadBatch
		 Description: 批量上传文件，自动识别每个文件的类型并推导默认purpose，
		 单个文件失败不影响其他文件的上传
		 Parameters:
			 - const std::vector<std::string>& file_paths: 本地文件路径数组
			 - const std::string& url: 文件接口的完整URL（必填）
		 Return: 返回每个文件的上传结果数组（与传入路径一一对应）
		 ============================================================================
		*/
		std::vector<FileOperator::FileUploadResult> UploadBatch(
			const std::vector<std::string>& file_paths,
			const std::string& url);

		/*
		 ============================================================================
		 Function: ToMessages
		 Description: 将多个文件转换为可直接用于对话的messages数组（高层封装，内部使用策略模式），
		 文档/音频类文件：上传(file-extract)并抽取内容，生成system消息；
		 图片类文件：base64编码为image_url内容part；
		 视频类文件：上传(purpose=video)并通过文件ID引用为video_url内容part；
		 所有媒体part最终合并为一条user消息。
		 各类型文件的处理策略可通过FileStrategyFactory::RegisterStrategy自定义替换
		 Parameters:
			 - const std::vector<std::string>& file_paths: 本地文件路径数组
			 - const std::string& url: 文件接口的完整URL（必填，策略内部的上传统一走此URL）
		 Return: 返回messages数组，建议将用户问题追加到该数组末尾后再发起对话
		 ============================================================================
		*/
		nlohmann::json ToMessages(const std::vector<std::string>& file_paths,
			const std::string& url);

		/*
		 ============================================================================
		 Function: List
		 Description: 获取文件列表（OpenAI兼容的 GET /v1/files 接口）
		 Parameters:
			 - const std::string& url: 文件接口的完整URL（必填）
		 Return: 返回服务器回复的json，失败返回空json对象
		 ============================================================================
		*/
		nlohmann::json List(const std::string& url);

		/*
		 ============================================================================
		 Function: Info
		 Description: 获取指定文件的详细信息（OpenAI兼容的 GET /v1/files/{file_id} 接口）
		 Parameters:
			 - const std::string& file_id: 文件ID（上传文件时服务器返回的id）
			 - const std::string& url: 文件接口的完整URL（必填）
		 Return: 返回服务器回复的json，失败返回空json对象
		 ============================================================================
		*/
		nlohmann::json Info(const std::string& file_id, const std::string& url);

		/*
		 ============================================================================
		 Function: Content
		 Description: 获取指定文件的内容（OpenAI兼容的 GET /v1/files/{file_id}/content 接口），
		 KIMI（Moonshot）对purpose为"file-extract"的文件返回解析后的文本内容，
		 此处返回原始字符串，由调用方决定是否解析
		 Parameters:
			 - const std::string& file_id: 文件ID（上传文件时服务器返回的id）
			 - const std::string& url: 文件接口的完整URL（必填）
		 Return: 返回原始响应字符串，失败返回空字符串
		 ============================================================================
		*/
		std::string Content(const std::string& file_id, const std::string& url);

		/*
		 ============================================================================
		 Function: Delete
		 Description: 删除指定文件（OpenAI兼容的 DELETE /v1/files/{file_id} 接口）
		 Parameters:
			 - const std::string& file_id: 文件ID（上传文件时服务器返回的id）
			 - const std::string& url: 文件接口的完整URL（必填）
		 Return: 返回服务器回复的json，失败返回空json对象
		 ============================================================================
		*/
		nlohmann::json Delete(const std::string& file_id, const std::string& url);

	private:
		/*
		 ============================================================================
		 Function: ParseRawToJson
		 Description: 将原始响应字符串解析为json对象（内部辅助函数），解析失败时根据错误抛出方式处理错误
		 ============================================================================
		*/
		// 注意：FileGateway 的所有错误统一经 m_ai.DoErrorThrow 抛出，
		// 以复用AI对象上配置的错误策略（网关自身不持有错误配置）
		nlohmann::json ParseRawToJson(const std::string& raw);

		AI& m_ai;		// 宿主AI对象引用（网关自身零状态）
	};

	class AI : public ThrowError {
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
		explicit AI() : Files(*this) {};

		/*
		 ============================================================================
		 Function: AI
		 Description: 构造函数
		 Parameters:
			 - std::shared_ptr<IHttpTransport> transport: 一个共享指针，指向一个实现了IHttpTransport接口的对象，用于处理HTTP请求
			 - const std::string&: 一个字符串，表示API站的URL
			 - const std::string&: 一个字符串，表示API站的API Key
			 - const ALL_AI_ErrorThrow: 一个枚举值，表示错误抛出方式
		 Return: 无
		 ============================================================================
		*/
		explicit AI(std::shared_ptr<IHttpTransport> transport,
			const std::string& url,
			const std::string& api_key,
			const ALL_AI_ErrorThrow all_ai_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW) :
			Files(*this),
			m_transport(std::move(transport)),
			m_url(url),
			m_api_key(api_key)
		{
			this->m_error_throw_method = all_ai_error_throw;
		}

		// 领域子对象：文件网关（公有值成员，与AI同生共死；自身零状态，URL由调用方传入）
		FileGateway Files;

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
		 Function: SetErrorThrow
		 Description: 设置错误抛出方式
		 Parameters:
			 -  ALL_AI_ErrorThrow: 一个枚举值，表示错误抛出方式
		 Return: 无返回值
		 ============================================================================
		*/
		void SetErrorThrow(ALL_AI_ErrorThrow error_throw)
		{
			this->m_error_throw_method = error_throw;
			return;
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
#if __ALL_AI_CXX_VERSION >= 17L
			std::scoped_lock lock(this->m_mutex_ai_init, this->m_mutex_config);
#elif (__ALL_AI_CXX_VERSION < 17L && __ALL_AI_CXX_VERSION >= 11L)
			std::lock_guard<std::mutex> lock_config(this->m_mutex_config);
			std::lock_guard<std::mutex> lock_init(this->m_mutex_ai_init);
#else
			return false;
#endif


			// 如果初始化过则直接返回false，表示不需要重复初始化
			// 如果URL、API Key或HTTP传输接口未设置，根据错误抛出方式处理错误并返回false
			if (this->m_initialized == true ||
				this->m_url.empty() || this->m_api_key.empty() || this->m_transport == nullptr)
			{
				DoErrorThrow("The API station URL, API key, or HTTP transmission interface is empty. Please check the configuration");
				return false;
			}
			// 设置构建器的错误抛出方式
			if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
				this->m_callback_function != nullptr)
			{
				// 这里不需要加锁，因为InitAI不与SendRequest并发（由用户保证或m_initialized标志）
				// 且Builder内部有锁
				this->m_builder.SetThrowErrorCallbackFunction(this->m_callback_function);
			}

			// 初始化HTTP传输接口
			if (this->m_transport)
			{
				this->m_initialized = this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw_method);
				return this->m_initialized;
			}
			return false;
		}

		/*
		============================================================================
		Function: ReloadAI
		Description: 重新加载AI，进行一些必要的设置和准备工作，
		Parameters:
			- std::string url: API站的URL，如果不为空则更新URL
			- std::string api_key: API密钥，如果不为空则更新API密钥
			- std::shared_ptr<IHttpTransport> transport: HTTP传输接口，如果不为空则更新HTTP传输接口
		Return: 返回一个布尔值，表示初始化是否成功。如果初始化成功，返回true；如果初始化失败，返回false
		============================================================================
	   */
		bool ReloadAI(std::string url = "",
			std::string api_key = "",
			std::shared_ptr<IHttpTransport> transport = std::make_shared<HttpTransport::CurlHttpTransport>())
		{
#if __ALL_AI_CXX_VERSION >= 17L
			std::scoped_lock lock(this->m_mutex_ai_init, this->m_mutex_config);
#elif (__ALL_AI_CXX_VERSION < 17L && __ALL_AI_CXX_VERSION >= 11L)
			std::lock_guard<std::mutex> lock_config(this->m_mutex_config);
			std::lock_guard<std::mutex> lock_init(this->m_mutex_ai_init);
#else
			return false;
#endif

			// 如果某个参数为空，返回false。以避免错误配置；如果不为空，则更新配置
			if (false == url.empty())
			{
				this->m_url = url;
			}
			if (false == api_key.empty())
			{
				this->m_api_key = api_key;
			}
			if (nullptr != transport)
			{
				this->m_transport = std::move(transport);
			}

			// 重新初始化Transport
			return this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw_method);
		}

		/*
		 ============================================================================
		 Function: SendRequest
		 Description: 发送HTTP请求，将用户的请求数据转换为JSON格式，并通过HTTP传输接口发送给服务器，然后接收服务器的回复并返回给用户
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
				DoErrorThrow("AI: HTTP transport is not set");
				return nlohmann::json{};
			}

			nlohmann::json result = transport_local->SendRequest(method, request_json);
			return result;
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
			return SendRequest(HttpMethod::POST, this->m_builder.BuilderToJson());
		}

		/*
		 ============================================================================
		 Function: SetDataCallback
		 Description: 设置数据回调：之后的请求收到响应数据块时逐块交给回调处理（不再收集），
		 用于音频二进制流（如TTS）、SSE、大文件下载等场景。
		 传入空DataCallback（或调用ClearDataCallback）恢复默认的收集行为。
		 回调是用户的资产，库不检查交付的数据
		 Parameters:
			 - DataCallback data_callback: 数据回调（空回调表示恢复默认行为）
		 Return: 无返回值
		 ============================================================================
		*/
		void SetDataCallback(DataCallback data_callback)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_config);
			this->m_data_callback = std::move(data_callback);
			return;
		}

		/*
		 ============================================================================
		 Function: ClearDataCallback
		 Description: 清除数据回调，恢复默认的"收完响应再解析"行为
		 Parameters:
			 - 无参数: 无释义
		 Return: 无返回值
		 ============================================================================
		*/
		void ClearDataCallback()
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_config);
			this->m_data_callback = nullptr;
			return;
		}

		/*
		 ============================================================================
		 Function: SendRequestRaw
		 Description: 发送普通HTTP请求并返回原始响应字符串（不做JSON解析），
		 若已设置数据回调，响应逐块交给回调并返回空字符串。
		 这是一切特殊端点（文件管理、TTS等）的机制层，
		 URL一律由调用方显式传入
		 Parameters:
			 - HttpMethod method: HTTP请求方法
			 - const std::string& url: 请求的完整URL
		 Return: 返回原始响应字符串（设置了回调时为空），失败返回空字符串
		 ============================================================================
		*/
		std::string SendRequestRaw(HttpMethod method, const std::string& url)
		{
			std::shared_ptr<IHttpTransport> transport_local;
			DataCallback callback_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
				callback_local = this->m_data_callback;
			}

			// 如果HTTP传输接口未设置，根据错误抛出方式处理错误
			if (!transport_local)
			{
				DoErrorThrow("AI: HTTP transport is not set");
				return std::string{};
			}

			if (callback_local)
			{
				return transport_local->SendRequestRaw(method, url, nullptr, callback_local);
			}
			return transport_local->SendRequestRaw(method, url);
		}

		/*
		 ============================================================================
		 Function: SendRequestRaw
		 Description: 发送携带JSON请求体的普通HTTP请求（重载版本，TTS等接口需要），
		 若已设置数据回调，响应逐块交给回调并返回空字符串
		 Parameters:
			 - HttpMethod method: HTTP请求方法
			 - const std::string& url: 请求的完整URL
			 - const nlohmann::json& body: JSON请求体
		 Return: 返回原始响应字符串（设置了回调时为空），失败返回空字符串
		 ============================================================================
		*/
		std::string SendRequestRaw(HttpMethod method, const std::string& url, const nlohmann::json& body)
		{
			std::shared_ptr<IHttpTransport> transport_local;
			DataCallback callback_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
				callback_local = this->m_data_callback;
			}

			if (!transport_local)
			{
				DoErrorThrow("AI: HTTP transport is not set");
				return std::string{};
			}

			return transport_local->SendRequestRaw(method, url, &body, callback_local);
		}

		/*
		 ============================================================================
		 Function: SendMultipartRequest
		 Description: 发送multipart/form-data表单请求（文件上传或纯字段表单），
		 这是一切上传类特殊端点（文件上传、语音转写、音色上传等）的机制层，
		 URL与表单字段一律由调用方显式传入（库不预设字段名）
		 Parameters:
			 - const std::string& url: 请求的完整URL
			 - const std::string& file_path: 本地文件路径，留空表示纯字段multipart（不携带文件）
			 - const std::string& file_field_name: 表单中文件字段的名称（OpenAI兼容接口为"file"）
			 - const std::unordered_map<std::string, std::string>& form_fields: 其他表单字段
		 Return: 返回服务器回复的json，失败返回空json对象
		 ============================================================================
		*/
		nlohmann::json SendMultipartRequest(const std::string& url,
			const std::string& file_path,
			const std::string& file_field_name,
			const std::unordered_map<std::string, std::string>& form_fields)
		{
			std::shared_ptr<IHttpTransport> transport_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
			}

			if (!transport_local)
			{
				DoErrorThrow("AI: HTTP transport is not set");
				return nlohmann::json{};
			}

			return transport_local->SendMultipartRequest(url, file_path, file_field_name, form_fields);
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
			return this->m_builder.BuilderToJson();
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

		std::shared_ptr<IHttpTransport> m_transport;	// HTTP传输接口

		// 数据回调（由m_mutex_config保护）：非空时响应数据块逐块交给用户，不再收集
		DataCallback m_data_callback;

		std::mutex m_mutex_config;		// 配置互斥锁（保护 URL, Key, Transport, DataCallback）
		std::mutex m_mutex_ai_init;		// AI初始化互斥锁

		JsonOperator::JsonRequestBuilder m_builder;
		JsonOperatorTools m_tools;

		bool m_initialized = false;	// AI是否已初始化
	};

	// 每种文件类型对应一种处理策略，可通过工厂注册自定义策略以扩展新类型或覆盖默认行为
	namespace FileOperator {

		/*
		 ============================================================================
		 Class: IFileProcessStrategy
		 Description: 文件处理策略接口（策略模式），定义了将文件转换为对话消息/内容part的方法，
					 并提供各具体策略共用的辅助函数
		 ============================================================================
		*/
		class IFileProcessStrategy {
		public:
			virtual ~IFileProcessStrategy() = default;

			// 获取该策略对应的文件用途
			virtual FilePurpose GetPurpose() const = 0;

			/*
			 ============================================================================
			 Function: Process
			 Description: 处理文件并转换为对话消息或内容part
			 Parameters:
				 - AI& ai: AI对象引用，用于调用文件网关等接口
				 - const std::string& file_path: 本地文件路径
				 - const std::string& files_url: 文件接口的完整URL（策略内部的上传统一走此URL）
				 - nlohmann::json& out_messages: 输出参数，文本类内容追加为消息（如system消息）
				 - nlohmann::json& out_parts: 输出参数，媒体类内容追加为content part（如image_url）
			 Return: 处理成功返回true，否则返回false
			 ============================================================================
			*/
			virtual bool Process(AI& ai, const std::string& file_path, const std::string& files_url,
				nlohmann::json& out_messages, nlohmann::json& out_parts) = 0;

		protected:

			/*
			 ============================================================================
			 Function: ExtractFileId
			 Description: 从上传响应json中安全地提取文件ID（内部辅助函数）
			 Parameters:
				 - const nlohmann::json& upload_result: 上传文件的响应json
			 Return: 提取成功返回文件ID，否则返回空字符串
			 ============================================================================
			*/
			static std::string ExtractFileId(const nlohmann::json& upload_result)
			{
				if (upload_result.is_object() &&
					upload_result.contains("id") &&
					upload_result["id"].is_string())
				{
					return upload_result["id"].get<std::string>();
				}
				return "";
			}

			/*
			 ============================================================================
			 Function: ExtractTextContent
			 Description: 从Files.Content返回的原始字符串中提取文件文本内容（内部辅助函数），
						 响应为JSON时提取content字段，否则返回原始字符串
			 Parameters:
				 - const std::string& raw_content: Files.Content返回的原始字符串
			 Return: 返回文件的文本内容
			 ============================================================================
			*/
			static std::string ExtractTextContent(const std::string& raw_content)
			{
				try
				{
					nlohmann::json content_json = nlohmann::json::parse(raw_content);
					if (content_json.is_object() &&
						content_json.contains("content") &&
						content_json["content"].is_string())
					{
						return content_json["content"].get<std::string>();
					}
				}
				catch (const nlohmann::json::parse_error&)
				{
					// 解析失败说明响应不是JSON，直接返回原始字符串
				}
				return raw_content;
			}

			/*
			 ============================================================================
			 Function: UploadAndGetId
			 Description: 上传文件并提取文件ID（内部辅助函数）
			 Parameters:
				 - AI& ai: AI对象引用
				 - const std::string& file_path: 本地文件路径
				 - FilePurpose purpose: 文件用途
				 - const std::string& files_url: 文件接口的完整URL
			 Return: 上传成功返回文件ID，否则返回空字符串
			 ============================================================================
			*/
			static std::string UploadAndGetId(AI& ai, const std::string& file_path, FilePurpose purpose,
				const std::string& files_url)
			{
				nlohmann::json upload_result = ai.Files.Upload(file_path, purpose, files_url);
				return ExtractFileId(upload_result);
			}
		};

		/*
		 ============================================================================
		 Class: DocumentFileStrategy
		 Description: 文档/文本类文件处理策略：上传(file-extract)并抽取内容，生成system消息
		 ============================================================================
		*/
		class DocumentFileStrategy : public IFileProcessStrategy {
		public:
			virtual FilePurpose GetPurpose() const override
			{
				return FilePurpose::FileExtract;
			}

			virtual bool Process(AI& ai, const std::string& file_path, const std::string& files_url, nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				std::string file_id = UploadAndGetId(ai, file_path, GetPurpose(), files_url);
				if (file_id.empty())
				{
					return false;
				}

				std::string text = ExtractTextContent(ai.Files.Content(file_id, files_url));
				if (text.empty())
				{
					return false;
				}

				out_messages.push_back({ {"role", "system"}, {"content", text} });
				return true;
			}
		};

		/*
		 ============================================================================
		 Class: ImageFileStrategy
		 Description: 图片类文件处理策略：默认将图片base64编码为image_url内容part（单张图片推荐），
					 也可切换为上传(purpose=image)后通过文件ID引用（多次引用推荐）
		 ============================================================================
		*/
		class ImageFileStrategy : public IFileProcessStrategy {
		public:
			virtual FilePurpose GetPurpose() const override
			{
				return FilePurpose::Image;
			}

			/*
			 ============================================================================
			 Function: SetTransportMode
			 Description: 设置图片传入方式
			 Parameters:
				 - ImageTransportMode mode: Base64 - base64编码后放入消息（默认）；
					 UploadReference - 上传后通过文件ID引用
			 Return: 无返回值
			 ============================================================================
			*/
			void SetTransportMode(ImageTransportMode mode)
			{
				this->m_mode = mode;
				return;
			}

			virtual bool Process(AI& ai, const std::string& file_path, const std::string& files_url, nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				if (this->m_mode == ImageTransportMode::UploadReference)
				{
					// 上传(purpose=image)后通过文件ID引用
					std::string file_id = UploadAndGetId(ai, file_path, GetPurpose(), files_url);
					if (file_id.empty())
					{
						return false;
					}
					out_parts.push_back({
						{"type", "image_url"},
						{"image_url", {{"url", "ms://" + file_id}}}
						});
					return true;
				}

				// 默认：base64编码为data URL
				std::string base64_data = JsonOperatorTools::FileToBase64(file_path);
				if (base64_data.empty())
				{
					return false;
				}
				std::string mime = FileTypeDetector::GetMimeType(file_path);
				out_parts.push_back({
					{"type", "image_url"},
					{"image_url", {{"url", "data:" + mime + ";base64," + base64_data}}}
					});
				return true;
			}

		private:
			ImageTransportMode m_mode = ImageTransportMode::Base64;	// 图片传入方式
		};

		/*
		 ============================================================================
		 Class: VideoFileStrategy
		 Description: 视频类文件处理策略：上传(purpose=video)后通过文件ID引用为video_url内容part
		 ============================================================================
		*/
		class VideoFileStrategy : public IFileProcessStrategy {
		public:
			virtual FilePurpose GetPurpose() const override
			{
				return FilePurpose::Video;
			}

			virtual bool Process(AI& ai, const std::string& file_path, const std::string& files_url,
				nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				std::string file_id = UploadAndGetId(ai, file_path, GetPurpose(), files_url);
				if (file_id.empty())
				{
					return false;
				}
				out_parts.push_back({
					{"type", "video_url"},
					{"video_url", {{"url", "ms://" + file_id}}}
					});
				return true;
			}
		};

		/*
		 ============================================================================
		 Class: AudioFileStrategy
		 Description: 音频类文件处理策略：默认按file-extract处理（部分平台支持音频转写为文本），
						 生成system消息。如需其他方式（如OpenAI的input_audio内容part），
						 可通过FileStrategyFactory::RegisterStrategy注册自定义策略覆盖
		 ============================================================================
		*/
		class AudioFileStrategy : public IFileProcessStrategy {
		public:
			virtual FilePurpose GetPurpose() const override
			{
				return FilePurpose::FileExtract;
			}

			virtual bool Process(AI& ai, const std::string& file_path, const std::string& files_url,
				nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				std::string file_id = UploadAndGetId(ai, file_path, GetPurpose(), files_url);
				if (file_id.empty())
				{
					return false;
				}

				std::string text = ExtractTextContent(ai.Files.Content(file_id, files_url));
				if (text.empty())
				{
					return false;
				}

				out_messages.push_back({ {"role", "system"}, {"content", text} });
				return true;
			}
		};

		/*
		 ============================================================================
		 Class: FileStrategyFactory
		 Description: 文件处理策略工厂（工厂模式），根据文件类型创建对应的处理策略，
					 支持通过RegisterStrategy注册自定义策略以扩展新类型或覆盖默认行为（开闭原则）
		 ============================================================================
		*/
		class FileStrategyFactory {
		private:
#if __ALL_AI_CXX_VERSION >= 17L
			// C++17 inline static成员保证了头文件库的单一定义
			inline static std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>> m_custom_strategies;
			inline static std::mutex m_mutex_custom;
#elif __ALL_AI_CXX_VERSION >= 14L
			// C++14 静态成员需要在cpp文件中定义，头文件中只声明
			// 但是Header-Only库无法在cpp文件中定义，因此使用函数局部静态变量来实现单例模式
			static std::mutex& GetFSFMutex()
			{
				static std::mutex instance;
				return instance;
			}

			static std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>>& GetFSFStrategies()
			{
				static std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>> instance;
				return instance;
			}
#endif

		public:

			/*
			 ============================================================================
			 Function: Create
			 Description: 根据文件类型创建对应的处理策略，用户注册的自定义策略优先于默认策略
			 Parameters:
				 - FileType file_type: 文件类型
			 Return: 返回策略对象的共享指针
			 ============================================================================
			*/
			static std::shared_ptr<IFileProcessStrategy> Create(FileType file_type)
			{
				// 用户注册的自定义策略优先
#if __ALL_AI_CXX_VERSION >= 17L
				{
					std::lock_guard<std::mutex> lock(m_mutex_custom);
					auto it = m_custom_strategies.find(file_type);
					if (it != m_custom_strategies.end())
					{
						return it->second;
					}
				}
#elif __ALL_AI_CXX_VERSION >= 14L
				{
					std::lock_guard<std::mutex> lock(GetFSFMutex());
					std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>>& strategies = GetFSFStrategies();
					auto it = strategies.find(file_type);
					if (it != strategies.end())
					{
						return it->second;
					}
				}
#endif

				switch (file_type)
				{
				case FileType::Image:
					return std::make_shared<ImageFileStrategy>();
				case FileType::Video:
					return std::make_shared<VideoFileStrategy>();
				case FileType::Audio:
					return std::make_shared<AudioFileStrategy>();
				case FileType::Document:
				case FileType::Unknown:
				default:
					return std::make_shared<DocumentFileStrategy>();
				}
			}

			/*
			 ============================================================================
			 Function: RegisterStrategy
			 Description: 注册自定义策略，替换指定文件类型的默认处理策略，
						 传入nullptr可恢复默认策略
			 Parameters:
				 - FileType file_type: 文件类型
				 - std::shared_ptr<IFileProcessStrategy> strategy: 自定义策略对象
			 Return: 无返回值
			 ============================================================================
			*/
#if __ALL_AI_CXX_VERSION >= 17L
			static void RegisterStrategy(FileType file_type, std::shared_ptr<IFileProcessStrategy> strategy)
			{
				std::lock_guard<std::mutex> lock(m_mutex_custom);
				if (strategy == nullptr)
				{
					m_custom_strategies.erase(file_type);
				}
				else
				{
					m_custom_strategies[file_type] = std::move(strategy);
				}
				return;
			}
#elif __ALL_AI_CXX_VERSION >= 14L
			static void RegisterStrategy(FileType file_type, std::shared_ptr<IFileProcessStrategy> strategy)
			{
				std::lock_guard<std::mutex> lock(GetFSFMutex());
				std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>>& strategies = GetFSFStrategies();
				if (strategy == nullptr)
				{
					strategies.erase(file_type);
				}
				else
				{
					strategies[file_type] = std::move(strategy);
				}
				return;
			}
#endif
		};
	}

	/*
	 ============================================================================
	 Function: FileGateway 成员函数的实现（需要AI类与策略家族的完整定义，故置于文件末尾）
	 ============================================================================
	*/
	inline nlohmann::json FileGateway::Upload(const std::string& file_path,
		const std::string& purpose,
		const std::string& url)
	{
		// URL必填：端点是用户的资产，库不存储、不推导
		if (url.empty())
		{
			this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			return nlohmann::json{};
		}

		// 构建表单字段并发送multipart请求
		std::unordered_map<std::string, std::string> form_fields;
		form_fields["purpose"] = purpose;
		return this->m_ai.SendMultipartRequest(url, file_path, "file", form_fields);
	}

	inline nlohmann::json FileGateway::Upload(const std::string& file_path,
		FileOperator::FilePurpose purpose,
		const std::string& url)
	{
		return Upload(file_path, FileOperator::FileTypeDetector::PurposeToString(purpose), url);
	}

	inline std::vector<FileOperator::FileUploadResult> FileGateway::UploadBatch(
		const std::vector<std::string>& file_paths,
		const std::string& url)
	{
		std::vector<FileOperator::FileUploadResult> results;
		results.reserve(file_paths.size());

		for (const std::string& file_path : file_paths)
		{
			FileOperator::FileUploadResult upload_result;
			upload_result.file_path = file_path;
			upload_result.file_type = FileOperator::FileTypeDetector::DetectFileType(file_path);

			FileOperator::FilePurpose purpose =
				FileOperator::FileTypeDetector::GetDefaultPurpose(upload_result.file_type);
			upload_result.raw_response = Upload(file_path, purpose, url);

			// 安全提取文件ID，判断上传是否成功
			if (upload_result.raw_response.is_object() &&
				upload_result.raw_response.contains("id") &&
				upload_result.raw_response["id"].is_string())
			{
				upload_result.file_id = upload_result.raw_response["id"].get<std::string>();
				upload_result.success = true;
			}

			results.push_back(std::move(upload_result));
		}

		return results;
	}

	inline nlohmann::json FileGateway::ToMessages(const std::vector<std::string>& file_paths,
		const std::string& url)
	{
		if (url.empty())
		{
			this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			return nlohmann::json::array();
		}

		nlohmann::json messages = nlohmann::json::array();
		nlohmann::json media_parts = nlohmann::json::array();

		for (const std::string& file_path : file_paths)
		{
			FileOperator::FileType file_type = FileOperator::FileTypeDetector::DetectFileType(file_path);
			std::shared_ptr<FileOperator::IFileProcessStrategy> strategy =
				FileOperator::FileStrategyFactory::Create(file_type);

			if (strategy == nullptr || !strategy->Process(this->m_ai, file_path, url, messages, media_parts))
			{
				this->m_ai.DoErrorThrow("FileGateway: failed to process file: " + file_path);
			}
		}

		// 媒体类内容（图片/视频）统一合并为一条user消息的content parts
		if (!media_parts.empty())
		{
			messages.push_back({ {"role", "user"}, {"content", media_parts} });
		}

		return messages;
	}

	inline nlohmann::json FileGateway::List(const std::string& url)
	{
		if (url.empty())
		{
			this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			return nlohmann::json{};
		}

		std::string str_result = this->m_ai.SendRequestRaw(HttpMethod::GET, url);
		return ParseRawToJson(str_result);
	}

	inline nlohmann::json FileGateway::Info(const std::string& file_id, const std::string& url)
	{
		if (url.empty() || file_id.empty())
		{
			if (file_id.empty())
			{
				this->m_ai.DoErrorThrow("FileGateway: file_id is empty");
			}
			if (url.empty())
			{
				this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			}
			return nlohmann::json{};
		}

		std::string str_result = this->m_ai.SendRequestRaw(HttpMethod::GET, url + "/" + file_id);
		return ParseRawToJson(str_result);
	}

	inline std::string FileGateway::Content(const std::string& file_id, const std::string& url)
	{
		if (url.empty() || file_id.empty())
		{
			if (file_id.empty())
			{
				this->m_ai.DoErrorThrow("FileGateway: file_id is empty");
			}
			if (url.empty())
			{
				this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			}
			return std::string{};
		}

		return this->m_ai.SendRequestRaw(HttpMethod::GET, url + "/" + file_id + "/content");
	}

	inline nlohmann::json FileGateway::Delete(const std::string& file_id, const std::string& url)
	{
		if (url.empty() || file_id.empty())
		{
			if (file_id.empty())
			{
				this->m_ai.DoErrorThrow("FileGateway: file_id is empty");
			}
			if (url.empty())
			{
				this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			}
			return nlohmann::json{};
		}

		std::string str_result = this->m_ai.SendRequestRaw(HttpMethod::DELETE, url + "/" + file_id);
		return ParseRawToJson(str_result);
	}

	inline nlohmann::json FileGateway::ParseRawToJson(const std::string& raw)
	{
		if (raw.empty())
		{
			return nlohmann::json{};
		}

		try
		{
			return nlohmann::json::parse(raw);
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::string error_message = "Error: FileGateway: JSON parse failed. Response: " + raw + ", Error: " + e.what();
			this->m_ai.DoErrorThrow(error_message);
			return nlohmann::json{};
		}
	}

	/*
	 ============================================================================
	 Function: UploadFile / UploadFiles / FilesToMessages
	                  GetFileList / GetFileInfo / GetFileContent / DeleteFile
	 Description: 自由函数拼写（对 ai.Files 对应成员的一行转发），
				 供偏爱扁平风格的用户使用，两种风格可自由混用，无重复实现
	 Example: auto up = ALL_AI::UploadFile(ai, "a.txt", "file-extract", files_url);
	 ============================================================================
	*/
	inline nlohmann::json UploadFile(AI& ai, const std::string& file_path,
		const std::string& purpose,
		const std::string& url)
	{
		return ai.Files.Upload(file_path, purpose, url);
	}

	inline std::vector<FileOperator::FileUploadResult> UploadFiles(AI& ai,
		const std::vector<std::string>& file_paths,
		const std::string& url)
	{
		return ai.Files.UploadBatch(file_paths, url);
	}

	inline nlohmann::json FilesToMessages(AI& ai,
		const std::vector<std::string>& file_paths,
		const std::string& url)
	{
		return ai.Files.ToMessages(file_paths, url);
	}

	inline nlohmann::json GetFileList(AI& ai, const std::string& url)
	{
		return ai.Files.List(url);
	}

	inline nlohmann::json GetFileInfo(AI& ai, const std::string& file_id, const std::string& url)
	{
		return ai.Files.Info(file_id, url);
	}

	inline std::string GetFileContent(AI& ai, const std::string& file_id, const std::string& url)
	{
		return ai.Files.Content(file_id, url);
	}

	inline nlohmann::json DeleteFile(AI& ai, const std::string& file_id, const std::string& url)
	{
		return ai.Files.Delete(file_id, url);
	}

}

#define ALL_AI_TOOL_MESSAGE_ROLE_USER		(ALL_AI::JsonOperatorTools::Role::User)
#define ALL_AI_TOOL_MESSAGE_ROLE_ASSISTANT	(ALL_AI::JsonOperatorTools::Role::Assistant)
#define ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM		(ALL_AI::JsonOperatorTools::Role::System)

// undef __ALL_AI_CXX_STANDARD
#ifdef __ALL_AI_CXX_STANDARD
	#undef __ALL_AI_CXX_STANDARD
#endif

// undef __ALL_AI_CXX_VERSION
#ifdef __ALL_AI_CXX_VERSION
	#undef __ALL_AI_CXX_VERSION
#endif

// undef __ALL_AI_SYSTEM_MARKER
#ifdef __ALL_AI_SYSTEM_MARKER
    #undef __ALL_AI_SYSTEM_MARKER
#endif

#endif