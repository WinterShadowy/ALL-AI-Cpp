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
* 
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

#include <curl/curl.h>
#include <functional>
#include <optional>

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

// 跨平台弃用宏
#if defined(__cplusplus) && __cplusplus >= 201402L	// C++14 及以上：使用标准属性
#define DEPRECATED(msg) [[deprecated(msg)]]
#elif defined(__GNUC__) || defined(__clang__)	// GCC/Clang 扩展
#define DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER) // MSVC 扩展
#define DEPRECATED(msg) __declspec(deprecated(msg))
#else	// 未知编译器，忽略
#define DEPRECATED(msg)
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

		/*
		============================================================================
		Function: SetThrowErrorCallbackFunction
		Description: 设置错误抛出的回调函数
		Parameters:
			- std::function<void(const std::string_view& message)> callback_function: 一个接受错误信息的回调函数
		Return: 无返回值
		============================================================================
		*/
		void SetThrowErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_function)
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
		void DoErrorThrow(std::string_view message)
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
		std::function<void(const std::string_view& message)>	m_callback_function;
	};

	// 请求构建策略
	class IRequestBuilderStrategy : virtual public ThrowError {
	public:
		virtual ~IRequestBuilderStrategy() = default;
		DEPRECATED("GetBuilder is deprecated, please use BuilderToJson instead")
		virtual nlohmann::json GetBuilder() = 0;

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

			/*
			 ============================================================================
			 Function: GetBuilder
			 Description: 获取json构建器
			 Parameters:
				 - 无参数: 无释义
			 Return: 返回json
			 ============================================================================
			*/
			DEPRECATED("GetBuilder is deprecated, please use BuilderToJson instead")
			virtual nlohmann::json GetBuilder() override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_request);
				return this->m_request_json;
			}

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
			std::optional<_T_Value> GetArrayBack(Args... keys);

			// 获取数组头部元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename _T_Value, typename... Args>
			std::optional<_T_Value> GetArrayFront(Args... keys);

			// 获取数组指定索引处元素（如果路径不存在或不是数组或数组为空则失败）
			template <typename _T_Value, typename... Args>
			std::optional<_T_Value> GetArrayValue(size_t index, Args... keys);

			// 创建空数组
			template <typename... Args>
			bool CreateArray(Args... keys);

			// 创建空对象
			template <typename... Args>
			bool CreateObject(Args... keys);

			// 清空数组
			template <typename... Args>
			bool ClearArray(Args... keys);

		private:

			// 路径元素类型：可以是字符串键或数组索引
			using PathKey = std::variant<std::string, size_t, int>;

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
			if (std::holds_alternative<std::string>(lastKey))
			{
				(*parent)[std::get<std::string>(lastKey)] = value;
			}
			else 
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

			for (const PathKey& key : _path) 
			{
				std::visit([&](auto&& k) {
					using T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<T, std::string>) 
					{
						// 对象键访问
						if (!current->contains(k)) 
						{
							if (!_createMissing) 
							{
								current = nullptr;
								return;
							}
							(*current)[k] = nlohmann::json::object();
						}
						current = &(*current)[k];
					}
					else if constexpr (std::is_same_v<T, size_t>) 
					{
						// 数组索引访问
						if (!current->is_array()) 
						{
							if (!_createMissing || !current->is_null()) 
							{
								// 如果不是null且不是数组，且不允许创建，失败
								if (!current->is_null()) 
								{
									current = nullptr;
									return;
								}
							}
							// 将null转为数组
							*current = nlohmann::json::array();
						}

						// 确保数组足够长
						if (k >= current->size()) 
						{
							if (!_createMissing) 
							{
								current = nullptr;
								return;
							}
							// 扩展数组，用null填充中间空缺
							while (current->size() <= k) 
							{
								current->push_back(nullptr);
							}
						}
						current = &(*current)[k];
					}
				}, key);

				if (current == nullptr) 
				{
					return nullptr;
				}
			}

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

			for (const auto& key : _path) 
			{
				// 访问当前节点
				std::visit([&](auto&& k) {
					using T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<T, std::string>) 
					{
						if (!current->contains(k) || !current->is_object()) 
						{
							current = nullptr;
							return;
						}
						current = &(*current)[k];
					}
					else if constexpr (std::is_same_v<T, size_t>) 
					{
						if (!current->is_array() || k >= current->size()) 
						{
							current = nullptr;
							return;
						}
						current = &(*current)[k];
					}
				}, key);

				// 如果当前节点为nullptr，返回nullptr
				if (current == nullptr) 
				{
					return nullptr;
				}
			}

			return current;
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
		 Function: GetValue
		 Description: 获取json某个字段指定的值 - 接口
		 Parameters:
		   - _Keys...: 剩余键（可变参数包），长度可为 0，作为索引
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
		 Function: _getValue
		 Description: 获取json某个字段指定的值 - 接口中间层
		 Parameters:
		   - nlohmann::json&: 待获取值的 json 对象（按值传递，内部副本）
		   - _Key&&: 指定的键
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
					std::string err = "Array index out of bounds: " + std::to_string(key);
					DoErrorThrow(err);
					return _T_Type{};
				}
				return _json.at(key);
			}
			else
			{
				// 对象键
				if (!_json.contains(key))
				{
					std::string err = "Key not found: " + std::string(key);
					DoErrorThrow(err);
					return _T_Type{};
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
		   - nlohmann::json&: 待获取值的 json 对象
		   - _First&&: 路径上的第一个键
		   - Args&&...: 剩余键（可变参数包），长度可为 0
		 Return: 获取成功返回对应的类型的数据，否则返回一个空数据
		 ============================================================================
		*/
		template <typename _T_Type, typename _First, typename... Args>
		_T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			nlohmann::json next_json = nullptr;

			// 根据first的类型判断是数组索引还是对象键，并进行相应的访问和错误处理
			if constexpr (std::is_integral_v<std::decay_t<_First>>)
			{
				// 数组索引
				if (!_json.is_array() || first < 0 || first >= _json.size())
				{
					std::string err = "Array index out of bounds: " + std::to_string(first);
					DoErrorThrow(err);
					return _T_Type{};
				}
				next_json = _json.at(first);
			}
			else
			{
				// 对象键
				if (!_json.contains(first))
				{
					// 根据错误抛出方式处理错误
					std::string err = "Key not found: " + std::string(first);
					DoErrorThrow(err);
					return _T_Type{};
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
	class IHttpTransport : public ThrowError {
	public:
		IHttpTransport() = default;
		virtual ~IHttpTransport() = default;
		// 初始化HTTP传输接口，设置URL、API Key和错误抛出方式
		virtual bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
		// 发送HTTP请求
		virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;
	};

	namespace HttpTransport
	{
		// 基于libcurl的HTTP传输实现
		class CurlHttpTransport final : public IHttpTransport{
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
					DoErrorThrow("CurlHttpTransport: url or api_key is empty");
					return false;
				}
				this->m_url = url;
				this->m_key = api_key;
				this->m_error_throw_method = all_ai_error_throw;

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
						int index = choice.value("index", 0);
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

		private:

			CURL* m_curl = nullptr;		// libcurl句柄
			std::mutex m_mutex_curl_request;

			std::string m_url;	// API - URL
			std::string m_key;		// API - Key
		};
	}

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
		explicit AI() {};

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
			m_transport(std::move(transport)),
			m_url(url),
			m_api_key(api_key)
		{
			this->m_error_throw_method = all_ai_error_throw;
		}

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
			std::lock_guard<std::mutex> lock(this->m_mutex_ai_init);

			// 如果初始化过则直接返回false，表示不需要重复初始化
			// 如果URL、API Key或HTTP传输接口未设置，根据错误抛出方式处理错误并返回false
			if (this->m_initialized == true || 
				this->m_url.empty() || this->m_api_key.empty() || this->m_transport == nullptr)
			{
				DoErrorThrow("The API station URL, API key, or HTTP transmission interface is empty. Please check the configuration");
				return false;
			}
			// 设置构建器与解析器的错误抛出方式
			if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
				this->m_callback_function != nullptr)
			{
				// 这里不需要加锁，因为InitAI不与SendRequest并发（由用户保证或m_initialized标志）
				// 且Builder/Parser内部有锁
				this->m_builder.SetThrowErrorCallbackFunction(this->m_callback_function);
				this->m_parser.SetThrowErrorCallbackFunction(this->m_callback_function);
			}

			// 初始化HTTP传输接口
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				if (this->m_transport)
				{
					this->m_initialized = this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw_method);
					return this->m_initialized;
				}
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
			// 获取配置锁，更新配置
			std::lock_guard<std::mutex> lock(this->m_mutex_config);

			// 如果某个参数为空，返回false。以避免错误配置；如果不为空，则更新配置
			if (url.empty() || api_key.empty() || transport == nullptr)
			{
				DoErrorThrow("AI: ReloadAI failed due to empty url, api_key, or null transport");
				return false;
			}
			else
			{
				this->m_url = url;
				this->m_api_key = api_key;
				this->m_transport = std::move(transport);
			}

			// 重新初始化Transport
			return this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw_method);
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
				DoErrorThrow("AI: HTTP transport is not set");
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
			return SendRequest(HttpMethod::POST, this->m_builder.BuilderToJson());
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

		std::shared_ptr<IHttpTransport> m_transport;	// HTTP传输接口

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