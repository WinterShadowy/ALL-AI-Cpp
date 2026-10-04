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
*	I'm glad you're using it
*
* ====================================================================================================
*
* ====================================================================================================
*
*   Developer's notes:
*   1. The developer is not an AI specialist, and my abilities are limited. Thank you for your understanding.
*   2. The developer is currently seeking a job (major: Computer Science and Technology). If you would like to offer an opportunity, please contact me via the email below.
*		2.5 Fortunately, the developer has found a job.
*   3. Open-source license: MIT
*
*	Online documentation:
*		https://doc.cpluscottage.top/web/#/642380673
*		https://ai-cpp-docsify.cpluscottage.top/ (Discontinued)
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
// Operating system: Windows
#if __ALL_AI_SYSTEM_MARKER >= 0x80L
#include <windows.h>
#include <strsafe.h>

// The DELETE macro defined in windows.h conflicts with the HttpMethod::DELETE enumeration value, so the definition is cancelled here
// Note: If the user's code includes windows.h after this header file, they need to manually #undef DELETE again
#if (defined(DELETE))
#undef DELETE
#endif

// Win32API: DeleteFile conflicts with the function for deleting files specified on the API website
// To use Win32 API to delete files, please include windows.h after this header file
#if (defined(DeleteFile))
#undef DeleteFile
#endif

#elif (__ALL_AI_SYSTEM_MARKER >= 0x40L && __ALL_AI_SYSTEM_MARKER < 0x80L)
#include <stdlib.h>
#include <string.h>
#endif

// Determine the compiler
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
	// HTTP method enumeration. Currently only libcurl-based sessions are supported.
	enum class HttpMethod {
		POST,
		GET,
		DELETE
	};

	// Data callback: invoked chunk by chunk when response data arrives (for binary responses
	// such as audio streams, SSE, or large downloads). The return value follows the libcurl
	// write callback convention: return the number of bytes consumed; returning a value
	// different from the given size aborts the request
	using DataCallback = std::function<size_t(const char* data, size_t size)>;

	// Error reporting modes
	enum class ALL_AI_ErrorThrow {
		ALL_AI_PRINT_ERROR,		// Report errors by printing messages
		ALL_AI_CALLBACK_FUNCTION,		// Report errors through a callback function
		ALL_AI_EXCEPTION_THROWING,		// Report errors by throwing exceptions
		ALL_AI_NO_ERROR_THROW		// Do not report errors
	};

	class ThrowError {
	public:
		ThrowError() {};
		~ThrowError() {};


		/*
		============================================================================
		Function: SetThrowErrorCallbackFunction
		Description: Set the callback function used for error reporting
		Parameters:
			- std::function<void(const std::string_view& message)> callback_function: A callback that receives error messages
		Return: No return value
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
		Description: Perform error throwing operation
		Parameters:
			- std::string_view message: error message
		Return: No return value
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

	// Request builder strategy
	class IRequestBuilderStrategy : virtual public ThrowError {
	public:
		virtual ~IRequestBuilderStrategy() = default;

		virtual nlohmann::json BuilderToJson() = 0;
		virtual void ClearBuilder() = 0;
		virtual nlohmann::json GetEmptyBuilder() = 0;
	};

	// Response parser strategy
	class IResponseParserStrategy : virtual public ThrowError {
	public:
		virtual ~IResponseParserStrategy() = default;
		virtual void Parse(const nlohmann::json& response) = 0;
		virtual void Parse(const std::string& response) = 0;
		virtual nlohmann::json GetData() = 0;
	};

	// Classes and functions related to JSON operations
	namespace JsonOperator {

		// JSON Request Builder
		class JsonRequestBuilder : public IRequestBuilderStrategy {
		public:

			JsonRequestBuilder() {}

			virtual ~JsonRequestBuilder() override = default;

			/*
			 ============================================================================
			 Function: BuilderToJson
			 Description: Returns the JSON object
			 Parameters:
				 - None: No parameters
			 Return: Returns the nlohmann::json object representing the current state of the builder
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
			 Description: Clears the JSON object
			 Parameters:
				 - None: No parameters
			 Return: No return value
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
			 Description: Returns an empty JSON object
			 Parameters:
				 - None: No parameters
			 Return: Returns an empty JSON object
			 ============================================================================
			*/
			virtual nlohmann::json GetEmptyBuilder() override
			{
				return nlohmann::json{};
			}

			// Sets a value at a specific JSON field path
			template <typename _T_Value, typename... Args>
			bool SetValue(_T_Value value, Args... keys);

			// Appends to an array (creates array if path doesn't exist, fails if exists but is not an array), at the end of the array
			template <typename _T_Value, typename... Args>
			bool ArrayPushBack(_T_Value value, Args... keys);

			// Deletes the last element of an array (fails if path doesn't exist, is not an array, or array is empty)
			template <typename... Args>
			bool ArrayDeleteBack(Args... keys);

			// Appends an element to the front of an array (creates array if path doesn't exist, fails if exists but is not an array)
			template <typename _T_Value, typename... Args>
			bool ArrayPushFront(_T_Value value, Args... keys);

			// Deletes the first element of an array (fails if path doesn't exist, is not an array, or array is empty)
			template <typename... Args>
			bool ArrayDeleteFront(Args... keys);

			// Append an element at the specified index in the array
			template <typename _T_Value, typename... Args>
			bool ArrayInsert(size_t index, _T_Value value, Args... keys);

			// Delete the element at the specified index in the array (if the path does not exist, or if it is not an array, or if the array is empty, the operation will fail)
			template <typename... Args>
			bool ArrayDelete(size_t index, Args... keys);

			// Inserts or replaces a value at a specific array index
			template <typename _T_Value, typename... Args>
			bool SetArrayValue(_T_Value value, size_t index, Args... keys);

			// Gets array length (returns 0 if path doesn't exist, -1 if not an array)
			template <typename... Args>
			int GetArrayLength(Args... keys);

			// Get the last element of the array (fail if the path does not exist, or if the input is not an array, or if the array is empty)
			template <typename _T_Value, typename... Args>
#if __ALL_AI_CXX_VERSION >= 17L
			std::optional<_T_Value> GetArrayBack(Args... keys);
#elif __ALL_AI_CXX_VERSION >= 14L
			_T_Value GetArrayBack(Args... keys);
#endif

			// Get the first element of the array (fail if the path does not exist, or if the input is not an array, or if the array is empty)
			template <typename _T_Value, typename... Args>
#if __ALL_AI_CXX_VERSION >= 17L
			std::optional<_T_Value> GetArrayFront(Args... keys);
#elif __ALL_AI_CXX_VERSION >= 14L
			_T_Value GetArrayFront(Args... keys);
#endif

			// Retrieve the element at the specified index of the array (fail if the path does not exist, or if the input is not an array, or if the array is empty)
			template <typename _T_Value, typename... Args>
#if __ALL_AI_CXX_VERSION >= 17L
			std::optional<_T_Value> GetArrayValue(size_t index, Args... keys);
#elif __ALL_AI_CXX_VERSION >= 14L
			_T_Value GetArrayValue(size_t index, Args... keys);
#endif

			// Creates an empty array
			template <typename... Args>
			bool CreateArray(Args... keys);

			// Creates an empty object
			template <typename... Args>
			bool CreateObject(Args... keys);

			// Clear an array (returns false if path doesn't exist or is not an array)
			template <typename... Args>
			bool ClearArray(Args... keys);

			// Path element type: can be a string key or an array index
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

				// Default constructor (required for certain operations of vector)
				PathKey() :
					type(Type::Int),
					str_val(),
					size_val(0),
					int_val(0)
				{
				}

				// Conversion constructor, replacing the implicit construction of std::variant
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
			// Terminates recursion
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path);

			// String key
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const std::string& _key);

			// Array index
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const char* _key);

			// Array index (size_t or int)
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, size_t _index);
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, int _index);

			// Variadic parameter expansion
			template <typename T, typename... Rest>
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... _rest);

			// Converts variadic parameters to a path array
			template <typename... Args>
			std::vector<PathKey> BuildPath(Args&&... _args);

			// Navigates to or creates a node by path (auto-creates intermediate objects/arrays)
			nlohmann::json* NavigateOrCreate(nlohmann::json& _root, const std::vector<PathKey>& _path, bool _createMissing = true);

			// Navigates to a node by path (read-only, no creation)
			nlohmann::json* Navigate(nlohmann::json& _root, const std::vector<PathKey>& _path);

		private:
			nlohmann::json m_request_json;
			mutable std::mutex m_mutex_request;
		};

		/*
		 ============================================================================
		 Function: SetValue
		 Description: Sets a value at a specific JSON field path - Interface
		 Parameters:
			 - _T_Value: The value to be set
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
		 ============================================================================
		*/
		template <typename _T_Value, typename... Args>
		inline bool JsonRequestBuilder::SetValue(_T_Value value, Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			// Uses new NavigateOrCreate to replace original recursive _setValue
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

			// Sets value, std::holds_alternative checks variable type, returns false if not string or size_t
			// true - string, key
			// false - size_t, index
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
				// For Int type or other unknown types, return false to maintain consistency with the original logic
				return false;
			}
#endif
			return true;
		}

		/*
		 ============================================================================
		 Function: ArrayPushBack
		 Description: Append elements to the end of the JSON array
		 Parameters:
			 - _T_Value: The value that needs to be set
			 - _Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the setting is successful, return true; otherwise, return false
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
		 Description: Deletes the last element of a JSON array
		 Parameters:
			 - Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the deletion is successful, return true; otherwise, return false
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
		 Description: Appends an element to the beginning of a JSON array
		 Parameters:
			 - _T_Value: The value to be set
			 - Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the setting is successful, return true; otherwise, return false
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
		 Description: Deletes the first element of a JSON array
		 Parameters:
			 - _T_Value: The value to be set
			 - Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the deletion is successful, return true; otherwise, return false
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
		 Description: Inserts an element at the specified index in a JSON array
		 Parameters:
			 - _T_Value: The value to be set
			 - size_t: The index
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
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
		 Description: Delete the element at the specified index in the JSON array
		 Parameters:
			 - size_t: The index
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
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
		Description: Sets a value at a specific JSON array index
		Parameters:
			- _T_Value: The value to be set
			- size_t: The index
			- Args...: Variadic parameters, must be strings, used as JSON field indices
		Return: Returns true on success, false otherwise
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

			// Ensures index is valid
			if (index > node->size())
			{
				// Expands array
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
		 Description: Gets the length of a JSON array
		 Parameters:
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Array length. Returns length if parameters are valid, -1 otherwise, 0 if node doesn't exist
		 ============================================================================
		*/
		template <typename... Args>
		inline int JsonRequestBuilder::GetArrayLength(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			// If the node does not exist, return 0
			if (node == nullptr)
			{
				return 0;
			}

			// If it is not an array, return -1
			if (!node->is_array())
			{
				return -1;
			}

			return static_cast<int>(node->size());
		}

		/*
		 ============================================================================
		 Function: GetArrayBack
		 Description: Gets the last element of a JSON array
		 Parameters:
			 - Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the retrieval is successful, return std::optional<_T_Value>; otherwise, return std::nullopt
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
		 Description: Gets the first element of a JSON array
		 Parameters:
			 - Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the retrieval is successful, return std::optional<_T_Value>; otherwise, return std::nullopt
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
		 Description: Gets the value at the specified index of a JSON array
		 Parameters:
			 - size_t: The index
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns std::optional<_T_Value> on success, std::nullopt otherwise
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
		 Description: Creates a JSON array
		 Parameters:
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
		 ============================================================================
		*/
		template <typename... Args>
		inline bool JsonRequestBuilder::CreateArray(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			// Node doesn't exist
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
		 Description: Creates a JSON object
		 Parameters:
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
		 ============================================================================
		*/
		template <typename... Args>
		inline bool JsonRequestBuilder::CreateObject(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = NavigateOrCreate(m_request_json, path, true);

			// Node doesn't exist
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
		 Description: Clears a JSON array
		 Parameters:
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: returns false if path doesn't exist or is not an array, true on success
		 ============================================================================
		*/
		template <typename... Args>
		inline bool JsonRequestBuilder::ClearArray(Args... keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(keys...);
			nlohmann::json* node = Navigate(m_request_json, path);

			// if node doesn't exist, return false
			if (node == nullptr)
			{
				return false;
			}

			// if not an array, return false
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
		 Description: Builds path - Recursion termination
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>& path: The path
		 Return: None
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path)
		{
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: Path building implementation
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: The path
			 - const std::string: The key
		 Return: None
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
		 Description: Path building implementation
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: The path
			 - const char*: The key
		 Return: None
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
		 Description: Path building implementation
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: The path
			 - size_t: The index
		 Return: None
		 ============================================================================
		*/
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, size_t _index)
		{
			_path.emplace_back(_index);
		}

		/*
		 ============================================================================
		 Function: BuildPathImpl
		 Description: Path building implementation
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: The path
			 - int: The index
		 Return: None
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
		 Description: Path building implementation
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>&: The path
			 - T&&: Parameter - First parameter
			 - Rest&&...: Parameters - Remaining parameters
		 Return: None
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
		 Description: Builds the path
		 Parameters:
			 - Args&&... args: Parameters
		 Return: Returns the path
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
		 Description: Navigates to a JSON node and creates it if missing
		 Parameters:
			 - nlohmann::json& root: Root node
			 - const std::vector<PathKey>& path: The path
			 - bool: Whether to create if path doesn't exist. true - create, false - do not create
		 Return: nlohmann::json*, returns nullptr if path doesn't exist, otherwise returns node pointer
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
						// Current node must be an object; null can be converted to an object when creation is allowed
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
							// Fill in null rather than object, so a later index key has a chance to turn it into an array
							(*current)[k] = nullptr;
						}
						current = &(*current)[k];
					}
					else if constexpr (std::is_same_v<_Key_T, size_t> || std::is_same_v<_Key_T, int>)
					{
						// For int indexes, check non-negativity first to avoid implicit conversion to a huge size_t
						if constexpr (std::is_same_v<_Key_T, int>)
						{
							if (k < 0)
							{
								current = nullptr;
								return;
							}
						}
						const size_t idx = static_cast<size_t>(k);

						// Current node must be an array; null can be converted to an array when creation is allowed
						if (!current->is_array())
						{
							if (!_createMissing || !current->is_null())
							{
								current = nullptr;
								return;
							}
							*current = nlohmann::json::array();
						}

						// Ensure the array is long enough, padding with null when it falls short
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

					// Current node must be an object; null can be converted to an object when creation is allowed
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
					// For int indexes, check non-negativity first to avoid implicit conversion
					if (key.type == PathKey::Type::Int && key.int_val < 0)
					{
						return nullptr;
					}

					size_t idx = (key.type == PathKey::Type::SizeT) ? key.size_val : static_cast<size_t>(key.int_val);

					// Current node must be an array; null can be converted to an array when creation is allowed
					if (!current->is_array())
					{
						if (!_createMissing || !current->is_null())
						{
							return nullptr;
						}
						*current = nlohmann::json::array();
					}

					// Ensure the array is long enough, padding with null when it falls short
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
				}		// Unknown type

				default:
					return nullptr;  // Unknown type
				}
			}
#endif
			return current;
		}

		/*
		 ============================================================================
		 Function: Navgate
		 Description: Navigates to a JSON node
		 Parameters:
			 - nlohmann::json& root: Root node
			 - const std::vector<PathKey>& path: The path
		 Return: nlohmann::json*, returns nullptr if path doesn't exist, otherwise returns node pointer
		 ============================================================================
		*/
		inline nlohmann::json* JsonRequestBuilder::Navigate(nlohmann::json& _root, const std::vector<PathKey>& _path)
		{
			nlohmann::json* current = &_root;
#if __ALL_AI_CXX_VERSION >= 17L
			for (const auto& key : _path)
			{
				// Access the current node
				std::visit([&](auto&& k) {
					using _Key_T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<_Key_T, std::string>)
					{
						// Must be an object and the key must exist, otherwise the lookup fails
						if (!current->is_object() || !current->contains(k))
						{
							current = nullptr;
							return;
						}
						current = &(*current)[k];
					}
					else if constexpr (std::is_same_v<_Key_T, size_t> || std::is_same_v<_Key_T, int>)
					{
						// For int indexes, check non-negativity first to avoid implicit conversion to a huge size_t
						if constexpr (std::is_same_v<_Key_T, int>)
						{
							if (k < 0)
							{
								current = nullptr;
								return;
							}
						}
						const size_t idx = static_cast<size_t>(k);

						// Must be an array and the index must be within range, otherwise the lookup fails
						if (!current->is_array() || idx >= current->size())
						{
							current = nullptr;
							return;
						}
						current = &(*current)[idx];
					}
					}, key);

				// If the current node is nullptr, return nullptr
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

					// Must be an object and the key must exist, otherwise the lookup fails
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
					// For int indexes, check non-negativity first to avoid implicit conversion to a huge size_t
					if (key.type == PathKey::Type::Int && key.int_val < 0)
					{
						return nullptr;
					}

					size_t idx = (key.type == PathKey::Type::SizeT)
						? key.size_val
						: static_cast<size_t>(key.int_val);

					// Must be an array and the index must be within range, otherwise the lookup fails
					if (!current->is_array() || idx >= current->size())
					{
						return nullptr;
					}
					current = &(*current)[idx];
					break;
				}		// Unknown type

				default:
					return nullptr;  // Unknown type
				}
			}
#endif
			return current;
		}

		// JSON parsing strategy
		class JsonResponseParser : public IResponseParserStrategy {
		public:

			JsonResponseParser() {}

			virtual ~JsonResponseParser() override = default;

			/*
			 ============================================================================
			 Function: Parse
			 Description: JSON parsing strategy
			 Parameters:
				 - nlohmann::json: A JSON object
			 Return: No return value
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
			 Description: JSON parsing strategy
			 Parameters:
				 - const std::string&: A JSON string
			 Return: No return value
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
			 Description: Get the parsed JSON data
			 Parameters:
				 - None: No parameters
			 Return: Returns a JSON object containing the parsed data
			 ============================================================================
			*/
			virtual nlohmann::json GetData() override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_response);
				return this->m_response_json;
			}

			// Get the value of a specific field - overload
			template <typename _T_Type, typename... _Keys>
			_T_Type GetValue(_Keys... _keys);

		private:

			// Base case for recursively retrieving a JSON field value
			template <typename _T_Type>
			_T_Type _getValue(const nlohmann::json& _json);

			// Recursive intermediate case for retrieving a JSON field value
			template <typename _T_Type, typename _First, typename... Args>
			_T_Type _getValue(const nlohmann::json& _json, _First&& first, Args... rest);

#if __ALL_AI_CXX_VERSION >= 17L
			// TODO
#elif __ALL_AI_CXX_VERSION >= 14L
			// Get a JSON field value - recursive intermediate layer, array index version
			template <typename _T_Type, typename _Index, typename... Args>
			_T_Type _getValueStep(const nlohmann::json& _json, _Index first, std::true_type, Args... rest);
			// Get a JSON field value - recursive intermediate layer, object key version
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
		 Description: Get the value of a specific JSON field - interface
		 Parameters:
		   - _Keys...: Remaining keys (variadic arguments), which may be empty and are used as indexes or keys
		 Return: Returns a value of the specified type on success; otherwise returns a default value
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
		 Description: Get the value of a specified JSON field - interface termination layer
		 Parameters:
		   - nlohmann::json&: The JSON object from which to get the value
		 Return: Returns the specialized value on success, otherwise an empty (default-constructed) value
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
				// The field exists but its type does not match (e.g. content is null but is being extracted as a string),
				DoErrorThrow(e.what());
				return _T_Type{};
			}
		}

		/*
		 ============================================================================
		 Function: _getValue
		 Description: Get the value of a specified JSON field - interface intermediate layer
		 Writes only when all intermediate objects along the entire path already exist; otherwise it gives up and returns false.
		 Parameters:
		   - nlohmann::json&: The JSON object from which to get the value
		   - _First&&: The first key along the path
		   - Args&&...: Remaining keys (variadic parameter pack), length may be 0
		 Return: Returns the data of the corresponding type on success, otherwise empty data
		 ============================================================================
		*/
#if __ALL_AI_CXX_VERSION >= 17L
		template <typename _T_Type, typename _First, typename... Args>
		_T_Type JsonResponseParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			if constexpr (std::is_integral_v<std::decay_t<_First>>)
			{
				// Array index
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
				// Object key
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
			return _getValueStep<_T_Type>(_json, std::forward<_First>(first),		// Note: C++14 has no _v suffix
				std::is_integral<std::decay_t<_First>>{},  // Note: C++14 has no _v suffix
				std::forward<Args>(rest)...);
		}

		// Array index version (selected when _First is an integral type)
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

		// Object key version (selected when _First is not an integral type)
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
	 Description: Stateless JSON value access (free function): safely navigate the given
	 json by path in a single call, without the two-step Parse/GetValue of
	 JsonResponseParser. Path keys support strings (object keys) and integers
	 (array indices), identical to JsonResponseParser::GetValue. On a missing path
	 or type mismatch the configured error mode applies and T{} is returned
	 Parameters:
		 - const nlohmann::json& data: The json object to read from
		 - Args&&... keys: Path keys (strings / integers, variadic)
	 Return: Returns the value, or T{} on failure
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

	// Utility class for JSON operations
	class JsonOperatorTools {
	public:

		// Role enumeration
		enum class Role {
			System,
			User,
			Assistant
		};

		/*
		 ============================================================================
		 Function: JsonOperatorTools
		 Description: Constructor
		 Parameters:
			 - None: No parameters
		 Return: No return value
		 ============================================================================
		*/
		JsonOperatorTools() {}

		/*
		 ============================================================================
		 Function: ~JsonOperatorTools
		 Description: Destructor
		 Parameters:
			 - None: No parameters
		 Return: No return value
		 ============================================================================
		*/
		~JsonOperatorTools() {}

		/*
		 ============================================================================
		 Function: GetMessagesArray
		 Description: Get the message array
		 Parameters:
			 - None: No parameters
		 Return: Returns a message array
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
		 Description: Add a message to the message array
		 Parameters:
			 - const Role&: The message role
			 - const std::string&: The message content
		 Return: No return value
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
		 Description: Remove the last message from the message array
		 Parameters:
			 - None: No parameters
		 Return: No return value
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
		 Description: Encode binary data into a base64 string,
		 used to build base64 image messages for vision models (data:image/xxx;base64,...)
		 Parameters:
			 - const std::string& data: The binary data to encode
		 Return: Returns the base64-encoded string
		 ============================================================================
		*/
		static std::string Base64Encode(const std::string& data)
		{
			static const char base64_table[] =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

			std::string encoded;
			encoded.reserve(((data.size() + 2) / 3) * 4);

			// Process 3 bytes per group into 4 base64 characters; pad the final incomplete group with '='
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
		 Description: Read a local file (in binary mode) and encode it as a base64 string,
		 commonly used to feed local images to vision models
		 Parameters:
			 - const std::string& file_path: Local file path
		 Return: Returns the base64-encoded string on success, or an empty string if the file does not exist or is unreadable
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
		 Description: Convert a role to a string
		 Parameters:
			 - Role: The role
		 Return: Returns a string
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

	// File operation related classes and functions (file type detection, multimodal content building, etc.)
	// File processing strategies and the strategy factory are defined after the AI class
	// (strategies depend on the AI class interface)
	namespace FileOperator {		// Unknown type (handled as a document by default)

		// File type enumeration, determines the processing strategy for a file
		enum class FileType {		// Document/text: txt, md, pdf, doc, xls, ppt, csv, etc.
			Unknown,		// Image: jpg, png, gif, webp, bmp, heic, etc.
			Document,		// Video: mp4, mov, avi, webm, wmv, etc.
			Image,		// Audio: mp3, wav, m4a, flac, ogg, etc.
			Video,		// Video: mp4, mov, avi, webm, wmv, etc.
			Audio		// Audio: mp3, wav, m4a, flac, ogg, etc.
		};		// "file-extract": extract file content (document/text files)

		// File purpose enumeration, corresponds to the purpose field of the file API
		enum class FilePurpose {		// "image": upload an image for visual understanding
			FileExtract,		// "video": upload a video for video understanding
			Image,		// "batch": upload a JSONL file for batch jobs
			Video,			// "video": upload a video for video understanding
			Batch			// "batch": upload a JSONL file for batch tasks
		};		// Base64-encode and embed directly in the message (recommended for single images)

		// Image transport mode enumeration
		enum class ImageTransportMode {		// Upload (purpose=image) and reference by file ID (recommended when referenced multiple times)
			Base64,			// base64-encode and embed directly in the message (recommended for a single image)
			UploadReference	// upload (purpose=image) and reference by file ID (recommended when referenced multiple times)
		};		// Local file path

		// File upload result
		struct FileUploadResult {		// File ID returned by the server on successful upload
			std::string file_path;		// Detected file type
			std::string file_id;		// Whether the upload succeeded
			FileType file_type = FileType::Unknown;		// Raw server response
			bool success = false;							// whether the upload succeeded
			nlohmann::json raw_response;					// raw server response
		};

		/*
		 ============================================================================
		 Class: FileTypeDetector
		 Description: File type detector. Detects the file type by extension and derives the default
		 purpose and MIME type. All methods are static; no instantiation is required
					 All methods are static; no instantiation is required
		 ============================================================================
		*/
		class FileTypeDetector {
		public:

			/*
			 ============================================================================
			 Function: DetectFileType
			 Description: Detect the file type by file extension
			 Parameters:
				 - const std::string& file_path: File path
			 Return: Returns the detected file type, or FileType::Unknown if unrecognized
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
			 Description: Get the default purpose for a file type
			 Parameters:
				 - FileType file_type: File type
			 Return: Returns the default file purpose
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
					// Documents and unknown types default to content extraction; audio defaults to
					// file-extract (some platforms support audio transcription). For other handling,
					// register a custom strategy via FileStrategyFactory::RegisterStrategy
					return FilePurpose::FileExtract;
				}
			}

			/*
			 ============================================================================
			 Function: PurposeToString
			 Description: Convert a file purpose enum to the API purpose string
			 Parameters:
				 - FilePurpose purpose: File purpose
			 Return: Returns the purpose string
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
			 Description: Get the MIME type by file extension (used when building base64 data URLs)
			 Parameters:
				 - const std::string& file_path: File path
			 Return: Returns the MIME type string, or "application/octet-stream" if unrecognized
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
			 Description: Extract the file extension and convert it to lowercase (internal helper)
			 Parameters:
				 - const std::string& file_path: File path
			 Return: Returns the lowercase extension (without the dot), or an empty string if none
			 ============================================================================
			*/
			static std::string GetExtensionLower(const std::string& file_path)
			{
				size_t dot_pos = file_path.find_last_of('.');
				size_t sep_pos = file_path.find_last_of("/\\");

				// No dot, or the dot appears before the last path separator (belongs to a directory name)
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
		 Description: Multimodal content part builder (Builder pattern) with a fluent API,
		 e.g. ContentPartBuilder().AddText("Describe the image").AddImageBase64("a.jpg").BuildUserMessage()
					 Example: ContentPartBuilder().AddText("describe the image").AddImageBase64("a.jpg").BuildUserMessage()
		 ============================================================================
		*/
		class ContentPartBuilder {
		public:

			/*
			 ============================================================================
			 Function: ContentPartBuilder
			 Description: Constructor
			 Parameters:
				 - None: No parameters
			 Return: None
			 ============================================================================
			*/
			ContentPartBuilder()
				: m_parts(nlohmann::json::array())
			{
			}

			/*
			 ============================================================================
			 Function: AddText
			 Description: Add a text part
			 Parameters:
				 - const std::string& text: Text content
			 Return: Returns the builder itself for chaining
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
			 Description: Add a local image part (reads the file and base64-encodes it into a data URL)
			 Parameters:
				 - const std::string& file_path: Local image path
			 Return: Returns the builder itself for chaining. No part is added if the file cannot be read
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
			 Description: Add an already-uploaded image part by file ID reference
			 (the file must have been uploaded with purpose="image")
			 Parameters:
				 - const std::string& file_id: File ID
			 Return: Returns the builder itself for chaining
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
			 Description: Add an already-uploaded video part by file ID reference
			 (the file must have been uploaded with purpose="video")
			 Parameters:
				 - const std::string& file_id: File ID
			 Return: Returns the builder itself for chaining
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
			 Description: Add a local audio part (reads the file and base64-encodes it into
					 a data URL, an OpenAI-compatible audio_url content part). Used as audio
					 input for audio-capable multimodal models
			 Parameters:
				 - const std::string& file_path: Local audio path
			 Return: Returns the builder itself for chaining. No part is added if the file cannot be read
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
			 Description: Build the content parts array
			 Parameters:
				 - None: No parameters
			 Return: Returns the content parts array
			 ============================================================================
			*/
			nlohmann::json BuildParts() const
			{
				return m_parts;
			}

			/*
			 ============================================================================
			 Function: BuildUserMessage
			 Description: Build a complete user message (content is the parts array)
			 Parameters:
				 - None: No parameters
			 Return: Returns the user message JSON
			 ============================================================================
			*/
			nlohmann::json BuildUserMessage() const
			{		// Content parts array
				return { {"role", "user"}, {"content", m_parts} };
			}

		private:
			nlohmann::json m_parts;	// content parts array
		};
	}

	// Abstract HTTP transport interface that defines how HTTP requests are sent
	class IHttpTransport : public ThrowError {
	public:
		IHttpTransport() = default;
		virtual ~IHttpTransport() = default;
		// Initialize the HTTP transport interface with the URL, API key, and error handling mode
		virtual bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
		// Send an HTTP request
		virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;
		// Clear resources of the HTTP transmission interface
		virtual void ClearResource() = 0;

		/*
		 ============================================================================
		 Function: SendMultipartRequest
		 Description: Send a multipart/form-data request (file upload). The default implementation
		 reports "not supported"; concrete transport classes override it. This is a
		 virtual function (not pure virtual) so that existing user-defined transport
		 classes continue to compile without modification
		 Parameters:
			 - const std::string& url: Full URL of the file endpoint (e.g. https://api.moonshot.cn/v1/files)
			 - const std::string& file_path: Local file path
			 - const std::string& file_field_name: Name of the file field in the form ("file" for OpenAI-compatible APIs)
			 - const std::unordered_map<std::string, std::string>& form_fields: Additional form fields besides the file (e.g. purpose)
		 Return: Returns a nlohmann::json representing the server response
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
		 Description: Send a plain HTTP request and return the raw response string (no JSON parsing),
		 used for endpoints whose response may not be JSON (e.g. retrieving file content).
		 The default implementation reports "not supported"; concrete transport
		 classes override it
		 Parameters:
			 - HttpMethod method: HTTP method
			 - const std::string& url: Full URL of the request
		 Return: Returns the raw response string, or an empty string on failure
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
		 Description: Send a plain HTTP request (optionally with a JSON body) and deliver
		 the response chunk by chunk through a data callback, for endpoints that return
		 binary streams (e.g. TTS audio) or other non-JSON data. The default
		 implementation ignores the body and the callback and falls back to the
		 two-argument overload, so existing user-defined transports keep compiling
		 Parameters:
			 - HttpMethod method: HTTP method
			 - const std::string& url: Full URL of the request
			 - const nlohmann::json* body: Optional JSON request body, nullptr for none
			 - DataCallback data_callback: Data callback; an empty callback means
			 the whole response is collected and returned as a string
		 Return: Returns the raw response string (usually empty when a callback is set,
		 as the data belongs to the callback), or an empty string on failure
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
		// libcurl-based HTTP transport implementation
		class CurlHttpTransport final : public IHttpTransport {
		public:

			/*
			 ============================================================================
			 Function: CurlHttpTransport
			 Description: Constructor
			 Parameters:
				 - None: No parameters
			 Return: No return value
			 ============================================================================
			*/
			CurlHttpTransport()
			{
			}

			/*
			 ============================================================================
			 Function: ~CurlHttpTransport
			 Description: Destructor
			 Parameters:
				 - None: No parameters
			 Return: No return value
			 ============================================================================
			*/
			~CurlHttpTransport()
			{
				ClearResource();
			}

			/*
			 ============================================================================
			 Function: Initialize
			 Description: Initialize the HTTP transport interface
			 Parameters:
				 - const std::string& url: The URL for the HTTP request
				 - const std::string& api_key: The API key
				 - const ALL_AI_ErrorThrow all_ai_error_throw: The error handling mode
			 Return: Returns true on success; otherwise false
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

				// Determine whether it has been initialized
				// If it has been initialized, close the initialized libcurl
				if (this->m_curl != nullptr)
				{
					ClearResource();
				}

				// Initialize libcurl
				this->m_curl = curl_easy_init();
				if (!this->m_curl)
				{
					// If initialization fails, handle the error according to the selected error mode
					DoErrorThrow("CurlHttpTransport: curl_easy_init failed");
					return false;
				}

				if (this->m_curl)
				{
					// Set the URL
					curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());
					curl_easy_setopt(this->m_curl, CURLOPT_FOLLOWLOCATION, 1L);

					// Ignore SSL certificate verification
					curl_easy_setopt(this->m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
					curl_easy_setopt(this->m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
				}
				return true;
			}

			/*
			 ============================================================================
			 Function: SendRequest
			 Description: Send an HTTP request
			 Parameters:
			   - HttpMethod: The HTTP request method
			   - const nlohmann::json: A JSON object representing the request payload
			 Return: Returns a nlohmann::json object representing the response
			 ============================================================================
			*/
			virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// If initialization failed, return an empty JSON object when a request is made
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
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);		// libcurl 7.56.0 and above: clear any residual multipart state

				// Clear any residual request flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// Restore the default body length (-1 = strlen): a previous request (e.g. TTS)
				// may have set an explicit POSTFIELDSIZE; without this reset, later request
				// bodies would be truncated to the stale length and rejected with HTTP 400
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 and above: clear any residual multipart flags
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
#endif
				// Ensure the URL is the one set during initialization
				// (file-related requests temporarily switch the URL; this is a safety restore)
				curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());

				// Set the request payload
				// Use error_handler_t::replace instead of the default strict handler:
				// when user strings contain invalid UTF-8 bytes (a common MSVC issue when the source
				// file is saved as GBK and Chinese string literals become GBK bytes), serialization
				// replaces them with U+FFFD instead of throwing type_error.316
				std::string str_json = request_json.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
				if (method == HttpMethod::POST)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, str_json.c_str());
					// Set the body length explicitly (consistent with the cleanup logic)
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(str_json.size()));
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// Execute the request
				CURLcode res = curl_easy_perform(this->m_curl);		// Ensure we free headers
				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					curl_slist_free_all(headers); // Ensure we free headers
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

				// If standard JSON parsing fails, try to parse the response as SSE instead
				// When the POST request has `stream` set to true,
				// parsing with nlohmann::json::parse inside the try block will fail,
				// so an SSE parsing attempt is required
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
			 Description: Send a multipart/form-data request (file upload). Builds the form with
			 the libcurl mime API; compatible with OpenAI-style /v1/files upload endpoints
			 Parameters:
				 - const std::string& url: Full URL of the file endpoint (e.g. https://api.moonshot.cn/v1/files)
				 - const std::string& file_path: Local file path
				 - const std::string& file_field_name: Name of the file field in the form ("file" for OpenAI-compatible APIs)
				 - const std::unordered_map<std::string, std::string>& form_fields: Additional form fields besides the file (e.g. purpose)
			 Return: Returns a nlohmann::json representing the server response.
			 If the request fails, an empty nlohmann::json object is returned
			 ============================================================================
			*/
			virtual nlohmann::json SendMultipartRequest(const std::string& url,
				const std::string& file_path,
				const std::string& file_field_name,
				const std::unordered_map<std::string, std::string>& form_fields) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// If initialization failed, return an empty JSON object when a request is made
				if (this->m_curl == nullptr)
				{		// The mime API requires libcurl 7.56.0 or later
					DoErrorThrow("CurlHttpTransport: curl is not initialized or failed to initialize");
					return nlohmann::json{};
				}

#if LIBCURL_VERSION_NUM < 0x073800	// the mime API requires libcurl 7.56.0 or later
				DoErrorThrow("CurlHttpTransport: SendMultipartRequest requires libcurl 7.56.0 or later");
				return nlohmann::json{};
#else
				// Check that the local file exists and is readable (skipped when file_path
				// is empty, which means a fields-only multipart request)
				if (!file_path.empty())
				{
					std::ifstream file_check(file_path, std::ios::binary);
					if (!file_check.good())
					{
						DoErrorThrow("CurlHttpTransport: cannot open file: " + file_path);
						return nlohmann::json{};
					}
				}

				// Build the multipart form
				curl_mime* mime = curl_mime_init(this->m_curl);
				if (mime == nullptr)
				{
					DoErrorThrow("CurlHttpTransport: curl_mime_init failed");
					return nlohmann::json{};
				}

				// Add the file field; libcurl reads the file content and fills in the filename
				// automatically (skipped when file_path is empty)
				curl_mimepart* part = nullptr;
				if (!file_path.empty())
				{
					part = curl_mime_addpart(mime);
					curl_mime_name(part, file_field_name.c_str());
					curl_mime_filedata(part, file_path.c_str());
				}

				// Add other plain form fields (e.g. purpose=file-extract)
				for (const auto& field : form_fields)
				{
					part = curl_mime_addpart(mime);
					curl_mime_name(part, field.first.c_str());
					curl_mime_data(part, field.second.c_str(), CURL_ZERO_TERMINATED);
				}

				// Set request headers. The multipart Content-Type is generated automatically by
				// libcurl (including the boundary); never set it manually
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

				// Clear any residual request flags to avoid state pollution from previous requests
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// Restore the default body length (-1 = strlen), clearing any explicit
				// POSTFIELDSIZE left by a previous request
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_CUSTOMREQUEST, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 0L);

				// Set the file endpoint URL and the multipart form
				curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, mime);

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// Execute the request
				CURLcode res = curl_easy_perform(this->m_curl);

				// Restore the URL and form state so subsequent plain JSON requests are not affected
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

				// Parse the JSON response
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
			 Description: Send a plain HTTP request and return the raw response string (no JSON parsing),
			 used for endpoints such as file content or file list. The initialization URL
			 is restored after the request completes
			 Parameters:
				 - HttpMethod method: HTTP method
				 - const std::string& url: Full URL of the request
			 Return: Returns the raw response string, or an empty string on failure
			 ============================================================================
			*/
			virtual std::string SendRequestRaw(HttpMethod method, const std::string& url) override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// If initialization failed, return an empty string when a request is made
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

				// Set request headers
				struct curl_slist* headers = nullptr;
				if (this->m_key.empty())
				{
					DoErrorThrow("CurlHttpTransport: API key is empty");
					return std::string{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);		// libcurl 7.56.0 and above: clear any residual multipart state

				// Clear any residual request flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// Restore the default body length (-1 = strlen): a previous request (e.g. TTS)
				// may have set an explicit POSTFIELDSIZE; without this reset, later request
				// bodies would be truncated to the stale length and rejected with HTTP 400
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 and above: clear any residual multipart flags
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
#endif
				if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				// Set the target URL
				curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// Execute the request
				CURLcode res = curl_easy_perform(this->m_curl);

				// Restore the URL so subsequent plain JSON requests are not affected
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
			 Description: Send a plain HTTP request (optionally with a JSON body) and deliver
			 the response chunk by chunk through a data callback, for endpoints that return
			 binary streams (e.g. TTS audio) or other non-JSON data. The initialization
			 URL is restored after the request completes
			 Parameters:
				 - HttpMethod method: HTTP method
				 - const std::string& url: Full URL of the request
				 - const nlohmann::json* body: Optional JSON request body, nullptr for none
				 - DataCallback data_callback: Data callback; an empty callback means
				 the whole response is collected and returned as a string
			 Return: Returns the raw response string (empty when a callback is set),
			 or an empty string on failure
			 ============================================================================
			*/
			virtual std::string SendRequestRaw(HttpMethod method, const std::string& url,
				const nlohmann::json* body, DataCallback data_callback) override
			{
				// Without a body and without a callback, fall back to the two-argument overload
				if (body == nullptr && !data_callback)
				{
					return SendRequestRaw(method, url);
				}

				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);

				// If initialization failed, return an empty string when a request is made
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

				// Set request headers
				struct curl_slist* headers = nullptr;
				if (this->m_key.empty())
				{
					DoErrorThrow("CurlHttpTransport: API key is empty");
					return std::string{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());

				// Clear any residual request flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				// Restore the default body length (-1 = strlen): a previous request (e.g. TTS)
				// may have set an explicit POSTFIELDSIZE; without this reset, later request
				// bodies would be truncated to the stale length and rejected with HTTP 400
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, -1L);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 and above: clear any residual multipart flags
				curl_easy_setopt(this->m_curl, CURLOPT_MIMEPOST, nullptr);
#endif
				if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				// Attach the JSON request body (required by endpoints such as TTS)
				std::string str_body;
				if (body != nullptr)
				{
					str_body = body->dump();
					headers = curl_slist_append(headers, "Content-Type: application/json");
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, str_body.c_str());
					curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(str_body.size()));
				}
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// Set the target URL
				curl_easy_setopt(this->m_curl, CURLOPT_URL, url.c_str());

				// With a data callback the response is delivered chunk by chunk (no collection);
				// otherwise the whole response is collected and returned
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

				// Execute the request
				CURLcode res = curl_easy_perform(this->m_curl);

				// Restore the URL so subsequent plain JSON requests are not affected
				curl_easy_setopt(this->m_curl, CURLOPT_URL, this->m_url.c_str());
				curl_slist_free_all(headers);

				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					DoErrorThrow(error_message);
					return std::string{};
				}

				// Check HTTP response code
				// Note: with a callback set, str_Buffer is empty and the error message carries
				// no response body (the response was delivered to the callback)
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
			 Description: Clear HTTP transmission interface
			 Parameters:
				- No parameters: No explanation
			 Return: No return value
			 ============================================================================
			*/
			virtual void ClearResource() override
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_curl_request);
				if (this->m_curl != nullptr)
				{
					// Clean up libcurl
					curl_easy_cleanup(this->m_curl);
					this->m_curl = nullptr;
				}
				return;
			}
		private:

			/*
			 ============================================================================
			 Function: Trim
			 Description: Remove leading and trailing whitespace from a string
			 Parameters:
			   - const std::string& input: The input string
			 Return: The string after trimming leading and trailing whitespace
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
			 Description: Try to parse an SSE response
			 Parameters:
			   - const std::string& response: The response string
			   - nlohmann::json& json_result: The parsed JSON result
			 Return: bool: Whether parsing succeeds
			 ============================================================================
			*/
			static bool TryParseSseResponse(const std::string& response, nlohmann::json& json_result)
			{
				nlohmann::json chunks = nlohmann::json::array();

				// SSE responses usually begin with "data:" and end with "\n\n", which marks the end of an event.
				// Parse the response line by line, extract lines beginning with "data: ", and parse their contents as JSON.
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

					// Trim leading and trailing whitespace from the line
					line = Trim(line);
					if (line.empty() || line.rfind(":", 0) == 0)
					{
						continue;
					}

					// Use rfind to avoid issues when multiple "data:" prefixes appear
					if (line.rfind("data:", 0) != 0)
					{
						continue;
					}

					// Remove the "data: " prefix
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

				// If no chunks were collected, parsing failed
				if (chunks.empty())
				{
					return false;
				}

				json_result = chunks.back();
				json_result["sse_chunks"] = chunks;

				nlohmann::json merged_choices = nlohmann::json::array();
				std::unordered_map<int, size_t> choice_index_to_pos;

				// Ensure each choice index has a corresponding merged_choice object; create one if needed
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

				// Merge the choices from all chunks
				for (const nlohmann::json& chunk : chunks)
				{
					if (!chunk.is_object() || !chunk.contains("choices") || !chunk["choices"].is_array())
					{
						continue;
					}

					// Merge the choices in the current chunk
					for (const nlohmann::json& choice : chunk["choices"])
					{
						// Extract index safely, avoiding type_error from value() when the index field has an unexpected type
						int index = 0;
						if (choice.is_object() && choice.contains("index") && choice["index"].is_number_integer())
						{
							index = choice["index"].get<int>();
						}
						nlohmann::json& merged_choice = ensure_choice(index);

						// Merge delta
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

						// Merge text
						if (choice.contains("text") && choice["text"].is_string())
						{
							merged_choice["message"]["content"] =
								merged_choice["message"]["content"].get<std::string>() + choice["text"].get<std::string>();
						}

						// Merge finish_reason
						if (choice.contains("finish_reason"))
						{
							merged_choice["finish_reason"] = choice["finish_reason"];
						}
					}
				}

				// If merged_choices is not empty, assign it to json_result["choices"]
				if (!merged_choices.empty())
				{
					json_result["choices"] = merged_choices;
				}

				return true;
			}

			/*
			 ============================================================================
			 Function: WriteCallback
			 Description: libcurl write callback that appends downloaded data to the user-specified std::string
			 Parameters:
			   - contents: Pointer to the received data buffer
			   - size: Byte size of each data block
			   - nmemb: Number of data blocks
			   - userp: User-defined pointer; here it points to the std::string used to store data
			 Return: Returns the total number of bytes processed (size * nmemb). If the return value does not match the expected amount, libcurl treats it as an error and aborts the transfer
			 ============================================================================
			*/
			static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
			{
				size_t totalSize = size * nmemb;
				userp->append(static_cast<char*>(contents), totalSize);		// libcurl handle
				return totalSize;
			}		// API - URL

			/*
			 ============================================================================
			 Function: DataCallbackWriter
			 Description: libcurl write callback (data callback mode) that forwards each
			 received chunk to the user callback. The chunk is NOT stored; the user
			 callback's return value is passed back to libcurl (abort on mismatch)
			 Parameters:
			   - contents: Pointer to the received data buffer
			   - size: Byte size of each data block
			   - nmemb: Number of data blocks
			   - userp: User-defined pointer; here it points to the DataCallback object
			 Return: Returns the value returned by the user callback. If it does not match
			 the incoming byte count, libcurl treats it as an error and aborts the transfer
			 ============================================================================
			*/
			static size_t DataCallbackWriter(void* contents, size_t size, size_t nmemb, DataCallback* userp)
			{
				size_t total_size = size * nmemb;
				return (*userp)(static_cast<const char*>(contents), total_size);
			}

		private:		// API - Key

			CURL* m_curl = nullptr;		// libcurl handle
			std::mutex m_mutex_curl_request;

			std::string m_url;	// API - URL
			std::string m_key;		// API - Key
		};
	}

	// Forward declaration of the AI class (FileGateway holds a back reference to AI)
	class AI;

	/*
	 ============================================================================
	 Class: FileGateway
	 Description: File gateway (domain sub-object, a public value member of AI:
	 ai.Files). Encapsulates the OpenAI-compatible /v1/files REST resource
	 structure: upload / batch upload / files-to-messages / list / info /
	 content / delete. The gateway holds NO state of its own - every target
	 URL is passed explicitly by the caller (URLs are the user's asset;
	 the library never stores, derives, or maps endpoint URLs)
	 ============================================================================
	*/
	class FileGateway : public ThrowError {
	public:

		/*
		 ============================================================================
		 Function: FileGateway
		 Description: Constructor (injected by the AI class only; no default constructor)
		 Parameters:
			 - AI& ai: Reference to the hosting AI object (back reference, used only
			 to invoke the sending capability)
		 Return: No return value
		 ============================================================================
		*/
		explicit FileGateway(AI& ai) : m_ai(ai) {}
		~FileGateway() = default;
		FileGateway(const FileGateway&) = delete;
		FileGateway& operator=(const FileGateway&) = delete;

		/*
		 ============================================================================
		 Function: Upload
		 Description: Upload a file to the file endpoint (multipart/form-data).
		 KIMI (Moonshot) and similar stations share the OpenAI format; their
		 purpose is usually "file-extract"
		 Parameters:
			 - const std::string& file_path: Local file path
			 - const std::string& purpose: File purpose. "file-extract" for KIMI;
			 "assistants"/"fine-tune" etc. for OpenAI
			 - const std::string& url: Full URL of the file endpoint (required,
			 e.g. https://api.moonshot.cn/v1/files)
		 Return: Returns the server response json (usually containing the file id),
		 or an empty json object on failure
		 ============================================================================
		*/
		nlohmann::json Upload(const std::string& file_path,
			const std::string& purpose,
			const std::string& url);

		/*
		 ============================================================================
		 Function: Upload
		 Description: Upload a file to the file endpoint (FilePurpose enum overload)
		 Parameters:
			 - const std::string& file_path: Local file path
			 - FileOperator::FilePurpose purpose: File purpose enum
			 - const std::string& url: Full URL of the file endpoint (required)
		 Return: Returns the server response json, or an empty json object on failure
		 ============================================================================
		*/
		nlohmann::json Upload(const std::string& file_path,
			FileOperator::FilePurpose purpose,
			const std::string& url);

		/*
		 ============================================================================
		 Function: UploadBatch
		 Description: Upload multiple files in batch. The type of each file is detected
		 automatically and its default purpose is derived. The failure of one file
		 does not affect the others
		 Parameters:
			 - const std::vector<std::string>& file_paths: Array of local file paths
			 - const std::string& url: Full URL of the file endpoint (required)
		 Return: Returns an array of per-file upload results (one-to-one with the input paths)
		 ============================================================================
		*/
		std::vector<FileOperator::FileUploadResult> UploadBatch(
			const std::vector<std::string>& file_paths,
			const std::string& url);

		/*
		 ============================================================================
		 Function: ToMessages
		 Description: Convert multiple files into a messages array ready for chat (high-level
		 API, internally based on the strategy pattern). Document/audio files are uploaded
		 (file-extract) and extracted into system messages; image files are base64-encoded
		 into image_url content parts; video files are uploaded (purpose=video) and
		 referenced by file ID as video_url parts; all media parts are merged into a
		 single user message. Strategies can be customized via
		 FileStrategyFactory::RegisterStrategy
		 Parameters:
			 - const std::vector<std::string>& file_paths: Array of local file paths
			 - const std::string& url: Full URL of the file endpoint (required; all uploads
			 inside the strategies go through this URL)
		 Return: Returns the messages array. It is recommended to append the user question
		 to the end of this array before starting the chat
		 ============================================================================
		*/
		nlohmann::json ToMessages(const std::vector<std::string>& file_paths,
			const std::string& url);

		/*
		 ============================================================================
		 Function: List
		 Description: Get the list of uploaded files (OpenAI-compatible GET /v1/files)
		 Parameters:
			 - const std::string& url: Full URL of the file endpoint (required)
		 Return: Returns the server response json, or an empty json object on failure
		 ============================================================================
		*/
		nlohmann::json List(const std::string& url);

		/*
		 ============================================================================
		 Function: Info
		 Description: Get detailed information about a file (GET /v1/files/{file_id})
		 Parameters:
			 - const std::string& file_id: File ID (the id returned by the server when uploading)
			 - const std::string& url: Full URL of the file endpoint (required)
		 Return: Returns the server response json, or an empty json object on failure
		 ============================================================================
		*/
		nlohmann::json Info(const std::string& file_id, const std::string& url);

		/*
		 ============================================================================
		 Function: Content
		 Description: Get the content of a file (GET /v1/files/{file_id}/content).
		 KIMI (Moonshot) returns the extracted text for files uploaded with purpose
		 "file-extract"; the raw string is returned here and the caller decides
		 whether to parse it
		 Parameters:
			 - const std::string& file_id: File ID (the id returned by the server when uploading)
			 - const std::string& url: Full URL of the file endpoint (required)
		 Return: Returns the raw response string, or an empty string on failure
		 ============================================================================
		*/
		std::string Content(const std::string& file_id, const std::string& url);

		/*
		 ============================================================================
		 Function: Delete
		 Description: Delete a file (DELETE /v1/files/{file_id})
		 Parameters:
			 - const std::string& file_id: File ID (the id returned by the server when uploading)
			 - const std::string& url: Full URL of the file endpoint (required)
		 Return: Returns the server response json, or an empty json object on failure
		 ============================================================================
		*/
		nlohmann::json Delete(const std::string& file_id, const std::string& url);

	private:
		/*
		 ============================================================================
		 Function: ParseRawToJson
		 Description: Parse a raw response string into a JSON object (internal helper). On
		 parse failure the error is handled according to the configured error mode
		 ============================================================================
		*/
		// Note: all FileGateway errors are reported through m_ai.DoErrorThrow so that the
		// error mode configured on the AI object applies (the gateway itself holds no
		// error configuration)
		nlohmann::json ParseRawToJson(const std::string& raw);

		AI& m_ai;		// Hosting AI reference (the gateway itself is stateless)
	};

	class AI : public ThrowError {
	public:

		/*
		 ============================================================================
		 Function: AI
		 Description: Constructor
		 Parameters:
			 - None: No parameters
		 Return: No return value
		 ============================================================================
		*/
		explicit AI() : Files(*this) {};

		/*
		 ============================================================================
		 Function: AI
		 Description: Constructor
		 Parameters:
			 - std::shared_ptr<IHttpTransport> transport: A shared pointer to an object implementing `IHttpTransport`, used to handle HTTP requests
			 - const std::string&: A string representing the API endpoint URL
			 - const std::string&: A string representing the API key
			 - const ALL_AI_ErrorThrow: An enum value representing the error handling mode
		 Return: No return value
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

		// Domain sub-object: file gateway (public value member, shares the AI lifetime;
		// stateless itself - every URL is passed explicitly by the caller)
		FileGateway Files;

		/*
		 ============================================================================
		 Function: ~AI
		 Description: Destructor
		 Parameters:
			 - None: No parameters
		 Return: No return value
		 ============================================================================
		*/
		~AI() {};

		/*
		 ============================================================================
		 Function: SetErrorThrow
		 Description: Set the error reporting mode
		 Parameters:
			 -  ALL_AI_ErrorThrow: An enum value representing the error reporting mode
		 Return: No return value
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
		 Description: Set the API endpoint URL
		 Parameters:
			 - const std::string&: A string representing the API endpoint URL
		 Return: No return value
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
		 Description: Set the API key
		 Parameters:
			 - const std::string&: A string representing the API key
		 Return: No return value
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
		 Description: Set the HTTP transport interface
		 Parameters:
			 -
		 Return:
		 Return: No return value
		 ============================================================================
				 - std::shared_ptr<IHttpTransport>: The HTTP transport implementation to use
		*/
		void SetHttpTransport(std::shared_ptr<IHttpTransport> transport)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_config);
			// Check whether the provided transport interface is null
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
		Description: Initialize AI by performing the required setup, such as initializing the HTTP transport interface and configuring error handling
		Parameters:
			- None: No parameters
		Return: Returns true if initialization succeeds; otherwise returns false
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


			// If already initialized, return false directly, indicating that repeated initialization is not needed
			// If the URL, API key, or HTTP transport interface is not set, handle the error according to the error mode and return false
			if (this->m_initialized == true ||
				this->m_url.empty() || this->m_api_key.empty() || this->m_transport == nullptr)
			{
				DoErrorThrow("The API station URL, API key, or HTTP transmission interface is empty. Please check the configuration");
				return false;
			}
			// Set the error handling mode for the builder
			if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
				this->m_callback_function != nullptr)
			{
				// No lock is needed here because InitAI does not run concurrently with SendRequest
				// (guaranteed by the user or the m_initialized flag), and the Builder has an internal lock
				this->m_builder.SetThrowErrorCallbackFunction(this->m_callback_function);
			}

			// Initialize the HTTP transport interface (m_mutex_config is already held, no need to lock again)
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
		Description: Reload AI and perform any required setup updates
		Parameters:
			- std::string url: The API endpoint URL. If it is not empty, the URL is updated
			- std::string api_key: The API key. If it is not empty, the API key is updated
			- std::shared_ptr<IHttpTransport> transport: The HTTP transport interface. If it is not null, the transport is updated
		Return: Returns true if reinitialization succeeds; otherwise returns false
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

			// If a certain parameter is empty, return false to avoid incorrect configuration; if it is not empty, update the configuration
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

			// Reinitialize the transport
			return this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw_method);
		}

		/*
		 ============================================================================
		 Function: SendRequest
		 Description: Send an HTTP request by converting the user request data to JSON, passing it through the HTTP transport interface, and returning the server response
		 Parameters:
			 - HttpMethod method: The HTTP request method (1. POST, 2. GET)
			 - const nlohmann::json request_json: A nlohmann::json object representing the request payload
		 Return: Returns a nlohmann::json object representing the server response. If the request fails or the response is invalid, an empty nlohmann::json object is returned
		 ============================================================================
		*/
		nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json)
		{
			std::shared_ptr<IHttpTransport> transport_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
			}

			// If the HTTP transport interface is not set, handle the error according to the selected error mode
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
		 Description: Send a POST request
		 Parameters:
			 - const nlohmann::json request_json: A nlohmann::json object representing the request payload
		 Return: Returns a nlohmann::json object representing the server response. If the request fails or the response is invalid, an empty nlohmann::json object is returned
		 ============================================================================
		*/
		nlohmann::json SendRequest_POST(const nlohmann::json request_json)
		{
			return SendRequest(HttpMethod::POST, request_json);
		}

		/*
		 ============================================================================
		 Function: SendRequest_GET
		 Description: Send a GET request
		 Parameters:
			 - const nlohmann::json request_json: A nlohmann::json object representing the request payload
		 Return: Returns a nlohmann::json object representing the server response. If the request fails or the response is invalid, an empty nlohmann::json object is returned
		 ============================================================================
		*/
		nlohmann::json SendRequest_GET(const nlohmann::json request_json)
		{
			return SendRequest(HttpMethod::GET, request_json);
		}

		/*
		 ============================================================================
		 Function: SendRequestFromBuilder_Get
		 Description: Send a GET request
		 Parameters:
			 - None: No parameters
		 Return: Returns a nlohmann::json object representing the server response. If the request fails or the response is invalid, an empty nlohmann::json object is returned
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
		 Description: Send a POST request using the builder
		 Parameters:
			 - None: No parameters
		 Return: Returns a nlohmann::json object representing the server response. If the request fails or the response is invalid, an empty nlohmann::json object is returned
		 ============================================================================
		*/
		nlohmann::json SendRequestFromBuilder_Post()
		{
			// GetBuilder() already locks internally and returns a copy
			return SendRequest(HttpMethod::POST, this->m_builder.BuilderToJson());
		}

		/*
		 ============================================================================
		 Function: SetDataCallback
		 Description: Set a data callback: subsequent requests deliver response chunks to
		 the callback instead of collecting them (for binary responses such as TTS audio
		 streams, SSE, or large downloads). Pass an empty DataCallback (or call
		 ClearDataCallback) to restore the default collect-and-parse behavior.
		 The callback is a user asset; the library never inspects the delivered data
		 Parameters:
			 - DataCallback data_callback: Data callback (empty restores the default behavior)
		 Return: No return value
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
		 Description: Clear the data callback and restore the default
		 collect-and-parse behavior
		 Parameters:
			 - None: No parameters
		 Return: No return value
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
		 Description: Send a plain HTTP request to an arbitrary URL and return the raw
		 response string (no JSON parsing). If a data callback is set, the response
		 is delivered chunk by chunk to the callback and an empty string is returned.
		 This is the mechanism layer for all special endpoints (file management,
		 TTS, etc.); the URL is always passed explicitly by the caller
		 Parameters:
			 - HttpMethod method: HTTP method
			 - const std::string& url: Full URL of the request
		 Return: Returns the raw response string (empty when a callback is set),
		 or an empty string on failure
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

			// If the HTTP transport is not set, handle the error according to the configured error mode
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
		 Description: Send a plain HTTP request with a JSON request body (overload,
		 required by endpoints such as TTS). If a data callback is set, the response
		 is delivered chunk by chunk to the callback and an empty string is returned
		 Parameters:
			 - HttpMethod method: HTTP method
			 - const std::string& url: Full URL of the request
			 - const nlohmann::json& body: JSON request body
		 Return: Returns the raw response string (empty when a callback is set),
		 or an empty string on failure
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
		 Description: Send a multipart/form-data request (file upload or fields-only form).
		 This is the mechanism layer for all upload-like special endpoints (file upload,
		 speech-to-text, voice clone, etc.); the URL and the form fields are always
		 passed explicitly by the caller (the library does not presume field names)
		 Parameters:
			 - const std::string& url: Full URL of the request
			 - const std::string& file_path: Local file path; empty means a fields-only
			 multipart request (no file attached)
			 - const std::string& file_field_name: Name of the file field in the form
			 ("file" for OpenAI-compatible APIs)
			 - const std::unordered_map<std::string, std::string>& form_fields: Additional form fields
		 Return: Returns the server response json, or an empty json object on failure
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
		 Description: Get the builder
		 Parameters:
			 - None: No parameters
		 Return: Returns a reference to the builder
		 ============================================================================
		*/
		JsonOperator::JsonRequestBuilder& GetBuilder()
		{
			// No lock is required because m_builder is a member variable whose address does not change
			// and the methods of JsonRequestBuilder are thread-safe
			return this->m_builder;
		}

		/*
		 ============================================================================
		 Function: GetBuilderData
		 Description: Get the builder JSON data
		 Parameters:
			 - None: No parameters
		 Return: Returns the JSON data currently stored in the builder
		 ============================================================================
		*/
		nlohmann::json GetBuilderData()
		{
			return this->m_builder.BuilderToJson();
		}

		/*
		 ============================================================================
		 Function: GetTools
		 Description: Get the utility helper object
		 Parameters:
			 - None: No parameters
		 Return: Returns a reference to the utility object, which contains commonly used JSON helper functions and can help users build requests and parse responses more conveniently
					such as ChatTool, which help users build requests and parse responses more conveniently
		 ============================================================================
		*/
		JsonOperatorTools& GetTools()
		{
			return this->m_tools;
		}

	private:

		std::string m_url;	// API - URL
		std::string m_api_key;	// API - Key

		std::shared_ptr<IHttpTransport> m_transport;	// HTTP transport interface

		// Data callback (protected by m_mutex_config): when set, response chunks are
		// delivered to the user one by one instead of being collected
		DataCallback m_data_callback;

		std::mutex m_mutex_config;		// Configuration mutex (protects URL, Key, Transport, DataCallback)
		std::mutex m_mutex_ai_init;		// AI initialization mutex

		JsonOperator::JsonRequestBuilder m_builder;
		JsonOperatorTools m_tools;

		bool m_initialized = false;	// whether the AI has been initialized
	};

	// File processing strategies and strategy factory (Strategy pattern + Factory pattern).
	// Each file type maps to a processing strategy; users can register custom strategies
	// through the factory to support new types or override default behavior
	namespace FileOperator {

		/*
		 ============================================================================
		 Class: IFileProcessStrategy
		 Description: File processing strategy interface (Strategy pattern). Defines how a file is
		 converted into chat messages / content parts, and provides helper functions
		 shared by all concrete strategies
					 and provides helper functions shared by the concrete strategies
		 ============================================================================
		*/
		class IFileProcessStrategy {
		public:
			virtual ~IFileProcessStrategy() = default;

			// Get the file purpose corresponding to this strategy
			virtual FilePurpose GetPurpose() const = 0;

			/*
			 ============================================================================
			 Function: Process
			 Description: Process a file and convert it into a chat message or content part
			 Parameters:
				 - AI& ai: Reference to the AI object, used to call the file gateway etc.
				 - const std::string& file_path: Local file path
				 - const std::string& files_url: Full URL of the file endpoint (all uploads
				 inside the strategy go through this URL)
				 - nlohmann::json& out_messages: Output. Text content is appended as messages (e.g. system messages)
				 - nlohmann::json& out_parts: Output. Media content is appended as content parts (e.g. image_url)
			 Return: Returns true on success, false otherwise
			 ============================================================================
			*/
			virtual bool Process(AI& ai, const std::string& file_path, const std::string& files_url,
				nlohmann::json& out_messages, nlohmann::json& out_parts) = 0;

		protected:

			/*
			 ============================================================================
			 Function: ExtractFileId
			 Description: Safely extract the file ID from an upload response JSON (internal helper)
			 Parameters:
				 - const nlohmann::json& upload_result: Upload response JSON
			 Return: Returns the file ID on success, or an empty string otherwise
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
			 Description: Extract the file text content from the raw string returned by
			 Files.Content (internal helper). If the response is JSON, the content
			 field is extracted; otherwise the raw string is returned as-is
			 Parameters:
				 - const std::string& raw_content: Raw string returned by Files.Content
			 Return: Returns the text content of the file
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
					// Parse failure means the response is not JSON; return the raw string as-is
				}
				return raw_content;
			}

			/*
			 ============================================================================
			 Function: UploadAndGetId
			 Description: Upload a file through the file gateway and extract its file ID
			 (internal helper)
			 Parameters:
				 - AI& ai: Reference to the AI object
				 - const std::string& file_path: Local file path
				 - FilePurpose purpose: File purpose
				 - const std::string& files_url: Full URL of the file endpoint
			 Return: Returns the file ID on success, or an empty string otherwise
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
		 Description: Document/text file strategy: upload (file-extract) and extract the content
		 into a system message (the file-chat approach officially recommended by KIMI)
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
		 Description: Image file strategy: by default the image is base64-encoded into an
		 image_url content part (recommended for single images); it can also be switched
		 to uploading (purpose=image) and referencing by file ID (recommended when the
		 image is referenced multiple times)
					 It can also be switched to uploading (purpose=image) and referencing by file ID (recommended for multiple references)
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
			 Description: Set the image transport mode
			 Parameters:
				 - ImageTransportMode mode: Base64 - base64-encode into the message (default);
				 UploadReference - upload and reference by file ID
					 UploadReference - upload and reference by file ID
			 Return: No return value
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
					// Upload (purpose=image) and reference by file ID
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

				// Default: base64-encode into a data URL
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
				return true;		// Image transport mode
			}

		private:
			ImageTransportMode m_mode = ImageTransportMode::Base64;	// image transport mode
		};

		/*
		 ============================================================================
		 Class: VideoFileStrategy
		 Description: Video file strategy: upload (purpose=video) and reference by file ID
		 as a video_url content part
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
		 Description: Audio file strategy: handled as file-extract by default (some platforms
		 support audio transcription) and produces a system message. For other
		 approaches (e.g. OpenAI's input_audio content part), register a custom
		 strategy via FileStrategyFactory::RegisterStrategy to override this one
						 to generate a system message. For other approaches (e.g. OpenAI-style input_audio content parts),
						 register a custom strategy via FileStrategyFactory::RegisterStrategy to override it
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
		 Description: File processing strategy factory (Factory pattern). Creates the strategy for
		 a given file type. Custom strategies can be registered via RegisterStrategy to
		 support new types or override default behavior (open-closed principle)
					 Supports registering custom strategies via RegisterStrategy to extend new types or override default behavior (open-closed principle)
		 ============================================================================
		*/
		class FileStrategyFactory {
		private:
#if __ALL_AI_CXX_VERSION >= 17L
			// C++17 inline static members guarantee a single definition for a header-only library
			inline static std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>> m_custom_strategies;
			inline static std::mutex m_mutex_custom;
#elif __ALL_AI_CXX_VERSION >= 14L
			// In C++14, static members need to be defined in cpp files, and only declared in header files
			// However, a Header-Only library cannot be defined in a cpp file
			// so a function-local static variable is used to implement the singleton pattern
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
			 Description: Create the processing strategy for a file type. User-registered custom
			 strategies take precedence over the default ones
			 Parameters:
				 - FileType file_type: File type
			 Return: Returns a shared pointer to the strategy object
			 ============================================================================
			*/
			static std::shared_ptr<IFileProcessStrategy> Create(FileType file_type)
			{
				// User-registered custom strategies take precedence
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
			 Description: Register a custom strategy to replace the default strategy for a file
			 type. Pass nullptr to restore the default strategy
			 Parameters:
				 - FileType file_type: File type
				 - std::shared_ptr<IFileProcessStrategy> strategy: Custom strategy object
			 Return: No return value
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
	 Function: Implementations of the FileGateway member functions (placed at the end of
	 the file because they require the complete definitions of AI and the strategy family)
	 ============================================================================
	*/
	inline nlohmann::json FileGateway::Upload(const std::string& file_path,
		const std::string& purpose,
		const std::string& url)
	{
		// The URL is required: endpoints are the user's asset; the library never
		// stores or derives them
		if (url.empty())
		{
			this->m_ai.DoErrorThrow("FileGateway: url is empty, please pass the file endpoint url explicitly");
			return nlohmann::json{};
		}

		// Build the form fields and send the multipart request
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

			// Extract the file ID safely to determine whether the upload succeeded
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

		// Media content (images/videos) is merged into the content parts of a single user message
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
	 Description: Free-function spellings (one-line forwarders to the corresponding
	 ai.Files members), for users who prefer a flat style. The two styles can be
	 mixed freely; there is no duplicated implementation
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