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
*   Thank you for using this library.
*	！！！Translation from KimiAI！！！
* ====================================================================================================
*
*   Developer's notes:
*   1. The developer is not an AI specialist, and my abilities are limited. Thank you for your understanding.
*   2. The developer is currently seeking a job (major: Computer Science and Technology). If you would like to offer an opportunity, please contact me via the email below.
*   3. Open-source license: MIT
*
*   Online documentation: https://doc.cpluscottage.top/web/#/642380673
*   Personal blog: https://xunlizhili.com
*   Feedback / updates / contact email: about@wang-sz.cn
*
*   If this library helps you, please consider giving it a star. Your support is my greatest motivation!
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
#include <fstream>
#include <iterator>
#include <cctype>
#include <unordered_set>

#include <curl/curl.h>
#include <functional>
#include <optional>

#include "nlohmann/json.hpp"

// Windows
#if ((defined(_WIN32) || defined(_WIN64)) && defined(_MSC_VER))
#define WIN_MSVC_VER 0L
#include <windows.h>
#include <strsafe.h>

// The DELETE macro defined in windows.h conflicts with the HttpMethod::DELETE enumerator, so undefine it here
// Note: if user code includes windows.h again AFTER this header, #undef DELETE must be applied once more
#if (defined(DELETE))
#undef DELETE
#endif

// linux
#elif __linux__ 
#define LINUX_VER 1L
#include <stdlib.h>
#include <string.h>
#endif

// Cross-platform deprecation macro
#if defined(__cplusplus) && __cplusplus >= 201402L	// C++14 and above: use standard attribute
#define DEPRECATED(msg) [[deprecated(msg)]]
#elif defined(__GNUC__) || defined(__clang__)	// GCC/Clang extension
#define DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER) // MSVC extension
#define DEPRECATED(msg) __declspec(deprecated(msg))
#else	// Unknown compiler, ignore
#define DEPRECATED(msg)
#endif

namespace ALL_AI
{
	// HTTP method enumeration. Currently only libcurl-based sessions are supported.
	enum class HttpMethod {
		POST,
		GET,
		DELETE
	};

	// Error reporting modes
	enum class ALL_AI_ErrorThrow {
		ALL_AI_PRINT_ERROR,			// Report errors by printing messages
		ALL_AI_CALLBACK_FUNCTION,	// Report errors through a callback function
		ALL_AI_EXCEPTION_THROWING,	// Report errors by throwing exceptions
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
		Description: Perform error throwing operation
		Parameters:
			- std::string_view message: error message
		Return: No return value
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

	// Request builder strategy
	class IRequestBuilderStrategy : virtual public ThrowError{
	public:
		virtual ~IRequestBuilderStrategy() = default;
		DEPRECATED("GetBuilder is deprecated, please use BuilderToJson instead")
		virtual nlohmann::json GetBuilder() = 0;
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

			/*
			 ============================================================================
			 Function: GetBuilder
			 Description: Returns the JSON builder object
			 Parameters:
				 - None: No parameters
			 Return: Returns the JSON object
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
			std::optional<_T_Value> GetArrayBack(Args... keys);

			// Get the first element of the array (fail if the path does not exist, or if the input is not an array, or if the array is empty)
			template <typename _T_Value, typename... Args>
			std::optional<_T_Value> GetArrayFront(Args... keys);

			// Retrieve the element at the specified index of the array (fail if the path does not exist, or if the input is not an array, or if the array is empty)
			template <typename _T_Value, typename... Args>
			std::optional<_T_Value> GetArrayValue(size_t index, Args... keys);

			// Creates an empty array
			template <typename... Args>
			bool CreateArray(Args... keys);

			// Creates an empty object
			template <typename... Args>
			bool CreateObject(Args... keys);

			// Clear an array (returns false if path doesn't exist or is not an array)
			template <typename... Args>
			bool ClearArray(Args... keys);

		private:

			// Path element type: can be a string key or an array index
			using PathKey = std::variant<std::string, size_t, int>;

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
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... rest);

			// Converts variadic parameters to a path array
			template <typename... Args>
			std::vector<PathKey> BuildPath(Args&&... _args);

			// Navigates to or creates a node by path (auto-creates intermediate objects/arrays)
			nlohmann::json* NavigateOrCreate(nlohmann::json& _root, const std::vector<PathKey>& _path, bool _createMissing = true);

			// Navigates to a node by path (read-only, no creation)
			nlohmann::json* Navigate(nlohmann::json& _root, const std::vector<PathKey>& _path);

			// Sets a JSON field value: recursion termination layer
			template <typename T>
			bool _setValue(nlohmann::json& _json, T&& _val, const std::string& _key);

			// Sets a JSON field value: recursion intermediate layer
			template <typename T, typename... Args>
			bool _setValue(nlohmann::json& _json, T&& val, const std::string& _first, Args&&... _rest);

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
		 Description: Gets the first element of a JSON array
		 Parameters:
			 - Args...: The optional parameter must be a string, serving as an index pointing to the field in JSON
		 Return: If the retrieval is successful, return std::optional<_T_Value>; otherwise, return std::nullopt
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
		 Description: Gets the value at the specified index of a JSON array
		 Parameters:
			 - size_t: The index
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns std::optional<_T_Value> on success, std::nullopt otherwise
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
		inline void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... rest)
		{
			BuildPathImpl(_path, std::forward<T>(_first));
			BuildPathImpl(_path, std::forward<Rest>(rest)...);
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

			for (const PathKey& key : _path)
			{
				std::visit([&](auto&& k) {
					using T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<T, std::string>)
					{
						// Object key access
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
						// Array index access
						if (!current->is_array())
						{
							if (!_createMissing || !current->is_null())
							{
								// If not null and not array, and creation not allowed, fail
								if (!current->is_null())
								{
									current = nullptr;
									return;
								}
							}
							// Converts null to array
							*current = nlohmann::json::array();
						}

						// Ensures array is long enough
						if (k >= current->size())
						{
							if (!_createMissing)
							{
								current = nullptr;
								return;
							}
							// Expands array, filling gaps with null
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
		 Function: Navigate
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

			for (const auto& key : _path)
			{
				// Accesses current node
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

				// If current node is nullptr, returns nullptr
				if (current == nullptr)
				{
					return nullptr;
				}
			}

			return current;
		}


		// JSON parsing strategy
		class JsonResponceParser : public IResponseParserStrategy {
		public:

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
				return this->m_response_json;
			}

			// Get the value of a specific field - overload
			template <typename _T_Type, typename... _Keys>
			_T_Type GetValue(_Keys... _keys);

		private:

			// Base case for recursively retrieving a JSON field value
			template <typename _T_Type, typename _Key>
			_T_Type _getValue(const nlohmann::json& _json, _Key&& key);

			// Recursive intermediate case for retrieving a JSON field value
			template <typename _T_Type, typename _First, typename... Args>
			_T_Type _getValue(const nlohmann::json& _json, _First&& first, Args... rest);

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
		inline _T_Type JsonResponceParser::GetValue(_Keys... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_response);
			return _getValue<_T_Type>(this->m_response_json, std::forward<_Keys>(_keys)...);
		}

		/*
		 ============================================================================
		 Function: _getValue
		 Description: Base layer for retrieving the value of a specific JSON field
		 Parameters:
		   - nlohmann::json&: The JSON object from which the value is retrieved
		   - _Key&&: The specified key or index
		 Return: Returns a specialized value on success; otherwise returns a default-constructed value
		 ============================================================================
		*/
		template <typename _T_Type, typename _Key>
		inline _T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _Key&& key)
		{
			// When a field exists but its type does not match (e.g. extracting content as a string
			// while it is null), nlohmann's implicit conversion throws type_error. Catch it here and
			// handle it according to the configured error mode so the exception never escapes to user code
			try
			{
				if constexpr (std::is_integral_v<std::decay_t<_Key>>)
				{
					// Array index
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
					// Object key
					if (!_json.contains(key))
					{
						std::string err = "Key not found: " + std::string(key);
						DoErrorThrow(err);
						return _T_Type{};
					}
					return _json.at(key);
				}
			}
			catch (const nlohmann::json::exception& e)
			{
				DoErrorThrow(e.what());
				return _T_Type{};
			}
		}

		/*
		 ============================================================================
		 Function: _getValue
		 Description: Recursive intermediate layer for retrieving the value of a specific JSON field
		 Parameters:
		   - nlohmann::json&: The JSON object from which the value is retrieved
		   - _First&&: The first key or index in the path
		   - Args&&...: Remaining keys (variadic arguments), which may be empty
		 Return: Returns the requested type on success; otherwise returns a default value
		 ============================================================================
		*/
		template <typename _T_Type, typename _First, typename... Args>
		inline _T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			nlohmann::json next_json = nullptr;

			// Determine whether `first` is an array index or an object key and handle access errors accordingly
			if constexpr (std::is_integral_v<std::decay_t<_First>>)
			{
				// Array index
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
				// Object key
				if (!_json.contains(first))
				{
					// Handle the error according to the selected error mode
					std::string err = "Key not found: " + std::string(first);
					DoErrorThrow(err);
					return _T_Type{};
				}
				next_json = _json.at(first);
			}

			return _getValue<_T_Type>(next_json, std::forward<Args>(rest)...);
		}
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
	namespace FileOperator {

		// File type enumeration, determines the processing strategy for a file
		enum class FileType {
			Unknown,	// Unknown type (handled as a document by default)
			Document,	// Document/text: txt, md, pdf, doc, xls, ppt, csv, etc.
			Image,		// Image: jpg, png, gif, webp, bmp, heic, etc.
			Video,		// Video: mp4, mov, avi, webm, wmv, etc.
			Audio		// Audio: mp3, wav, m4a, flac, ogg, etc.
		};

		// File purpose enumeration, corresponds to the purpose field of the file API
		enum class FilePurpose {
			FileExtract,	// "file-extract": extract file content (document/text files)
			Image,			// "image": upload an image for visual understanding
			Video,			// "video": upload a video for video understanding
			Batch			// "batch": upload a JSONL file for batch jobs
		};

		// Image transport mode enumeration
		enum class ImageTransportMode {
			Base64,			// Base64-encode and embed directly in the message (recommended for single images)
			UploadReference	// Upload (purpose=image) and reference by file ID (recommended when referenced multiple times)
		};

		// File upload result
		struct FileUploadResult {
			std::string file_path;							// Local file path
			std::string file_id;							// File ID returned by the server on successful upload
			FileType file_type = FileType::Unknown;			// Detected file type
			bool success = false;							// Whether the upload succeeded
			nlohmann::json raw_response;					// Raw server response
		};

		/*
		 ============================================================================
		 Class: FileTypeDetector
		 Description: File type detector. Detects the file type by extension and derives the default
					 purpose and MIME type. All methods are static; no instantiation is required
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
					"ppt", "pptx", "csv", "json", "xml", "html", "htm", "epub"
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
					{"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"png", "image/png"},
					{"gif", "image/gif"}, {"webp", "image/webp"}, {"bmp", "image/bmp"},
					{"heic", "image/heic"}, {"heif", "image/heif"},
					{"mp4", "video/mp4"}, {"mpeg", "video/mpeg"}, {"mov", "video/quicktime"},
					{"avi", "video/x-msvideo"}, {"flv", "video/x-flv"}, {"mpg", "video/mpeg"},
					{"webm", "video/webm"}, {"wmv", "video/x-ms-wmv"}, {"3gpp", "video/3gpp"},
					{"mp3", "audio/mpeg"}, {"wav", "audio/wav"}, {"m4a", "audio/mp4"},
					{"flac", "audio/flac"}, {"ogg", "audio/ogg"}, {"aac", "audio/aac"},
					{"wma", "audio/x-ms-wma"}, {"pdf", "application/pdf"}, {"txt", "text/plain"}
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
			{
				return { {"role", "user"}, {"content", m_parts} };
			}

		private:
			nlohmann::json m_parts;	// Content parts array
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
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// Clear any residual request flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 and above: clear any residual multipart state
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
				}
				else if (method == HttpMethod::GET)
				{
					curl_easy_setopt(this->m_curl, CURLOPT_HTTPGET, 1L);
				}

				std::string str_Buffer;
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
				curl_easy_setopt(this->m_curl, CURLOPT_WRITEDATA, &str_Buffer);

				// Execute the request
				CURLcode res = curl_easy_perform(this->m_curl);
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
					DoErrorThrow("Error: Empty response received from server");
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
				{
					DoErrorThrow("CurlHttpTransport: curl is not initialized or failed to initialize");
					return nlohmann::json{};
				}

#if LIBCURL_VERSION_NUM < 0x073800	// The mime API requires libcurl 7.56.0 or later
				DoErrorThrow("CurlHttpTransport: SendMultipartRequest requires libcurl 7.56.0 or later");
				return nlohmann::json{};
#else
				// Check that the local file exists and is readable
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

				// Add the file field; libcurl reads the file content and fills in the filename automatically
				curl_mimepart* part = curl_mime_addpart(mime);
				curl_mime_name(part, file_field_name.c_str());
				curl_mime_filedata(part, file_path.c_str());

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
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// Clear any residual request flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);
#if LIBCURL_VERSION_NUM >= 0x073800	// libcurl 7.56.0 and above: clear any residual multipart state
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
			 Function: ClearResource
			 Description: Clear HTTP transmission interface
			 Parameters:
				- No parameters: No explanation
			 Return: No return value
			 ============================================================================
			*/
			virtual void ClearResource() override
			{
				if(this->m_curl != nullptr)
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
				userp->append(static_cast<char*>(contents), totalSize);
				return totalSize;
			}

		private:

			CURL* m_curl = nullptr;		// libcurl handle
			std::mutex m_mutex_curl_request;

			std::string m_url;	// API - URL
			std::string m_key;	// API - Key
		};
	}

	class AI : public ThrowError{
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
		explicit AI() {};

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
			m_transport(std::move(transport)),
			m_url(url),
			m_api_key(api_key)
		{
			this->m_error_throw_method = all_ai_error_throw;
		}

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
			 - std::shared_ptr<IHttpTransport>: The HTTP transport implementation to use
		 Return: No return value
		 ============================================================================
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
			std::lock_guard<std::mutex> lock(this->m_mutex_ai_init);

			// If AI has already been initialized, return false to avoid repeated initialization
			if (this->m_initialized == true ||
				this->m_url.empty() || this->m_api_key.empty() || this->m_transport == nullptr)
			{
				DoErrorThrow("The API station URL, API key, or HTTP transmission interface is empty. Please check the configuration");
				return false;
			}
			// Set the error handling mode for the builder and parser
			if (this->m_error_throw_method == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
				this->m_callback_function != nullptr)
			{
				// No extra lock is required here because InitAI does not run concurrently with SendRequest
				// (this is guaranteed by the user or by the m_initialized flag), and Builder/Parser already have internal locks
				this->m_builder.SetThrowErrorCallbackFunction(this->m_callback_function);
				this->m_parser.SetThrowErrorCallbackFunction(this->m_callback_function);
			}

			// Initialize the HTTP transport interface
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
			// Acquire the configuration lock and update the configuration
			std::lock_guard<std::mutex> lock(this->m_mutex_config);

			// If a certain parameter is empty, return false to avoid incorrect configuration; if it is not empty, update the configuration
			if (false == url.empty())
			{
				this->m_url = url;
			}
			if (false == api_key.empty())
			{
				this->m_api_key = api_key;
			}
			if(nullptr != transport)
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

			// SendRequest is thread-safe here because CurlHttpTransport is already protected by a lock
			// Parse is also thread-safe here because JsonResponceParser is already protected by a lock
			nlohmann::json result = transport_local->SendRequest(method, request_json);
			this->m_parser.Parse(result);
			return this->m_parser.GetData();
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
		 Function: UploadFile
		 Description: Upload a file to the file endpoint of the API station (OpenAI-compatible
					 /v1/files endpoint, multipart/form-data). KIMI (Moonshot) and similar stations
					 share the OpenAI format; their purpose is usually "file-extract"
		 Parameters:
			 - const std::string& file_path: Local file path
			 - const std::string& purpose: File purpose. "file-extract" for KIMI; "assistants"/"fine-tune"
				 etc. for OpenAI. Defaults to "file-extract"
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL (see MakeFilesURL)
		 Return: Returns a nlohmann::json object representing the server response (usually containing
				 the file id). If the request fails or the response is invalid, an empty object is returned
		 ============================================================================
		*/
		nlohmann::json UploadFile(const std::string& file_path,
			const std::string& purpose = "file-extract",
			const std::string& files_url = "")
		{
			std::shared_ptr<IHttpTransport> transport_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
			}

			// If the HTTP transport is not set, handle the error according to the configured error mode
			if (!transport_local)
			{
				DoErrorThrow("AI: HTTP transport is not set");
				return nlohmann::json{};
			}

			// Derive the file endpoint URL
			std::string target_url = MakeFilesURL(files_url);
			if (target_url.empty())
			{
				return nlohmann::json{};
			}

			// Build the form fields and send the multipart request
			std::unordered_map<std::string, std::string> form_fields;
			form_fields["purpose"] = purpose;
			nlohmann::json result = transport_local->SendMultipartRequest(target_url, file_path, "file", form_fields);
			this->m_parser.Parse(result);
			return this->m_parser.GetData();
		}

		/*
		 ============================================================================
		 Function: UploadFile
		 Description: Upload a file to the file endpoint (FilePurpose enum overload)
		 Parameters:
			 - const std::string& file_path: Local file path
			 - FileOperator::FilePurpose purpose: File purpose enum
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL
		 Return: Returns a nlohmann::json object representing the server response.
				 If the request fails or the response is invalid, an empty object is returned
		 ============================================================================
		*/
		nlohmann::json UploadFile(const std::string& file_path,
			FileOperator::FilePurpose purpose,
			const std::string& files_url = "")
		{
			return UploadFile(file_path, FileOperator::FileTypeDetector::PurposeToString(purpose), files_url);
		}

		/*
		 ============================================================================
		 Function: UploadFiles
		 Description: Upload multiple files in batch. The type of each file is detected automatically
					 and its default purpose is derived. The failure of one file does not affect the others
		 Parameters:
			 - const std::vector<std::string>& file_paths: Array of local file paths
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL
		 Return: Returns an array of per-file upload results (one-to-one with the input paths)
		 ============================================================================
		*/
		std::vector<FileOperator::FileUploadResult> UploadFiles(const std::vector<std::string>& file_paths,
			const std::string& files_url = "")
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
				upload_result.raw_response = UploadFile(file_path, purpose, files_url);

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

		/*
		 ============================================================================
		 Function: FilesToMessages
		 Description: Convert multiple files into a messages array ready for chat (high-level API,
					 internally based on the strategy pattern):
					 document/audio files: uploaded (file-extract) and extracted into system messages;
					 image files: base64-encoded into image_url content parts;
					 video files: uploaded (purpose=video) and referenced by file ID as video_url parts;
					 all media parts are finally merged into a single user message.
					 The per-type strategies can be customized via FileStrategyFactory::RegisterStrategy
		 Parameters:
			 - const std::vector<std::string>& file_paths: Array of local file paths
		 Return: Returns the messages array. It is recommended to append the user question to the
				 end of this array before starting the chat
		 ============================================================================
		*/
		nlohmann::json FilesToMessages(const std::vector<std::string>& file_paths);

		/*
		 ============================================================================
		 Function: GetFileList
		 Description: Get the list of files uploaded to the API station
					 (OpenAI-compatible GET /v1/files endpoint)
		 Parameters:
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL
		 Return: Returns a nlohmann::json object representing the server response.
				 If the request fails or the response is invalid, an empty object is returned
		 ============================================================================
		*/
		nlohmann::json GetFileList(const std::string& files_url = "")
		{
			std::string target_url = MakeFilesURL(files_url);
			if (target_url.empty())
			{
				return nlohmann::json{};
			}

			std::string str_result = SendFileRawRequest(HttpMethod::GET, target_url);
			nlohmann::json result = ParseRawToJson(str_result);
			this->m_parser.Parse(result);
			return this->m_parser.GetData();
		}

		/*
		 ============================================================================
		 Function: GetFileInfo
		 Description: Get detailed information about a specific file
					 (OpenAI-compatible GET /v1/files/{file_id} endpoint)
		 Parameters:
			 - const std::string& file_id: File ID (the id returned by the server when uploading)
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL
		 Return: Returns a nlohmann::json object representing the server response.
				 If the request fails or the response is invalid, an empty object is returned
		 ============================================================================
		*/
		nlohmann::json GetFileInfo(const std::string& file_id, const std::string& files_url = "")
		{
			std::string target_url = MakeFilesURL(files_url);
			if (target_url.empty() || file_id.empty())
			{
				if (file_id.empty())
				{
					DoErrorThrow("AI: file_id is empty");
				}
				return nlohmann::json{};
			}

			std::string str_result = SendFileRawRequest(HttpMethod::GET, target_url + "/" + file_id);
			nlohmann::json result = ParseRawToJson(str_result);
			this->m_parser.Parse(result);
			return this->m_parser.GetData();
		}

		/*
		 ============================================================================
		 Function: GetFileContent
		 Description: Get the content of a specific file
					 (OpenAI-compatible GET /v1/files/{file_id}/content endpoint).
					 For files uploaded with purpose "file-extract", KIMI (Moonshot) returns the
					 extracted text content; the response is usually a JSON string (with a content
					 field). The raw string is returned here and the caller decides whether to parse it
		 Parameters:
			 - const std::string& file_id: File ID (the id returned by the server when uploading)
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL
		 Return: Returns the raw response string, or an empty string on failure
		 ============================================================================
		*/
		std::string GetFileContent(const std::string& file_id, const std::string& files_url = "")
		{
			std::string target_url = MakeFilesURL(files_url);
			if (target_url.empty() || file_id.empty())
			{
				if (file_id.empty())
				{
					DoErrorThrow("AI: file_id is empty");
				}
				return std::string{};
			}

			return SendFileRawRequest(HttpMethod::GET, target_url + "/" + file_id + "/content");
		}

		/*
		 ============================================================================
		 Function: DeleteFile
		 Description: Delete a specific file on the API station
					 (OpenAI-compatible DELETE /v1/files/{file_id} endpoint)
		 Parameters:
			 - const std::string& file_id: File ID (the id returned by the server when uploading)
			 - const std::string& files_url: Full URL of the file endpoint. If empty, it is derived
				 automatically from the chat URL
		 Return: Returns a nlohmann::json object representing the server response.
				 If the request fails or the response is invalid, an empty object is returned
		 ============================================================================
		*/
		nlohmann::json DeleteFile(const std::string& file_id, const std::string& files_url = "")
		{
			std::string target_url = MakeFilesURL(files_url);
			if (target_url.empty() || file_id.empty())
			{
				if (file_id.empty())
				{
					DoErrorThrow("AI: file_id is empty");
				}
				return nlohmann::json{};
			}

			std::string str_result = SendFileRawRequest(HttpMethod::DELETE, target_url + "/" + file_id);
			nlohmann::json result = ParseRawToJson(str_result);
			this->m_parser.Parse(result);
			return this->m_parser.GetData();
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
		 Function: GetParser
		 Description: Get a parser
		 Parameters:
			 - None: No parameters
		 Return: Returns a reference to the parser
		 ============================================================================
		*/
		JsonOperator::JsonResponceParser& GetParser()
		{
			return this->m_parser;
		}

		/*
		 ============================================================================
		 Function: GetTools
		 Description: Get the utility helper object
		 Parameters:
			 - None: No parameters
		 Return: Returns a reference to the utility object, which contains commonly used JSON helper functions and can help users build requests and parse responses more conveniently
		 ============================================================================
		*/
		JsonOperatorTools& GetTools()
		{
			return this->m_tools;
		}

	private:

		/*
		 ============================================================================
		 Function: MakeFilesURL
		 Description: Build the full URL of the file endpoint. If the caller explicitly provides
					 files_url, it is used directly; otherwise it is derived from the chat URL by
					 taking everything before "/v1" and appending "/v1/files",
					 e.g. https://api.moonshot.cn/v1/chat/completions -> https://api.moonshot.cn/v1/files
		 Parameters:
			 - const std::string& files_url: File endpoint URL explicitly provided by the caller; may be empty
		 Return: Returns the full file endpoint URL, or an empty string if derivation fails
		 ============================================================================
		*/
		std::string MakeFilesURL(const std::string& files_url)
		{
			// The caller explicitly specified the file endpoint URL; use it directly
			if (!files_url.empty())
			{
				return files_url;
			}

			std::string url_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				url_local = this->m_url;
			}

			// Take everything before "/v1" and append "/v1/files"
			size_t pos = url_local.find("/v1");
			if (pos == std::string::npos)
			{
				DoErrorThrow("AI: cannot derive files url from chat url, please pass files_url explicitly");
				return std::string{};
			}
			return url_local.substr(0, pos) + "/v1/files";
		}

		/*
		 ============================================================================
		 Function: SendFileRawRequest
		 Description: Send a file-related request through the HTTP transport and return the raw
					 response string (internal helper)
		 Parameters:
			 - HttpMethod method: HTTP method
			 - const std::string& url: Full URL of the request
		 Return: Returns the raw response string, or an empty string on failure
		 ============================================================================
		*/
		std::string SendFileRawRequest(HttpMethod method, const std::string& url)
		{
			std::shared_ptr<IHttpTransport> transport_local;
			{
				std::lock_guard<std::mutex> lock(this->m_mutex_config);
				transport_local = this->m_transport;
			}

			// If the HTTP transport is not set, handle the error according to the configured error mode
			if (!transport_local)
			{
				DoErrorThrow("AI: HTTP transport is not set");
				return std::string{};
			}

			return transport_local->SendRequestRaw(method, url);
		}

		/*
		 ============================================================================
		 Function: ParseRawToJson
		 Description: Parse a raw response string into a JSON object (internal helper). On parse
					 failure the error is handled according to the configured error mode
		 Parameters:
			 - const std::string& raw: Raw response string
		 Return: Returns the parsed JSON object on success, or an empty JSON object on failure
		 ============================================================================
		*/
		nlohmann::json ParseRawToJson(const std::string& raw)
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
				std::string error_message = "Error: AI: JSON parse failed. Response: " + raw + ", Error: " + e.what();
				DoErrorThrow(error_message);
				return nlohmann::json{};
			}
		}

		std::string m_url;	// API - URL
		std::string m_api_key;	// API - Key

		std::shared_ptr<IHttpTransport> m_transport;	// HTTP transport

		std::mutex m_mutex_config;		// Configuration mutex (protects URL, Key, and Transport)
		std::mutex m_mutex_ai_init;		// AI initialization mutex

		JsonOperator::JsonRequestBuilder m_builder;
		JsonOperator::JsonResponceParser m_parser;
		JsonOperatorTools m_tools;

		bool m_initialized = false;	// Whether AI has been initialized
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
				 - AI& ai: Reference to the AI object, used to call upload / file content APIs
				 - const std::string& file_path: Local file path
				 - nlohmann::json& out_messages: Output. Text content is appended as messages (e.g. system messages)
				 - nlohmann::json& out_parts: Output. Media content is appended as content parts (e.g. image_url)
			 Return: Returns true on success, false otherwise
			 ============================================================================
			*/
			virtual bool Process(AI& ai, const std::string& file_path,
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
						 GetFileContent (internal helper). If the response is JSON, the content
						 field is extracted; otherwise the raw string is returned as-is
			 Parameters:
				 - const std::string& raw_content: Raw string returned by GetFileContent
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
			 Description: Upload a file and extract its file ID (internal helper)
			 Parameters:
				 - AI& ai: Reference to the AI object
				 - const std::string& file_path: Local file path
				 - FilePurpose purpose: File purpose
			 Return: Returns the file ID on success, or an empty string otherwise
			 ============================================================================
			*/
			static std::string UploadAndGetId(AI& ai, const std::string& file_path, FilePurpose purpose)
			{
				nlohmann::json upload_result = ai.UploadFile(file_path, purpose);
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

			virtual bool Process(AI& ai, const std::string& file_path,
				nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				std::string file_id = UploadAndGetId(ai, file_path, GetPurpose());
				if (file_id.empty())
				{
					return false;
				}

				std::string text = ExtractTextContent(ai.GetFileContent(file_id));
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
			 Return: No return value
			 ============================================================================
			*/
			void SetTransportMode(ImageTransportMode mode)
			{
				this->m_mode = mode;
				return;
			}

			virtual bool Process(AI& ai, const std::string& file_path,
				nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				if (this->m_mode == ImageTransportMode::UploadReference)
				{
					// Upload (purpose=image) and reference by file ID
					std::string file_id = UploadAndGetId(ai, file_path, GetPurpose());
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
				return true;
			}

		private:
			ImageTransportMode m_mode = ImageTransportMode::Base64;	// Image transport mode
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

			virtual bool Process(AI& ai, const std::string& file_path,
				nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				std::string file_id = UploadAndGetId(ai, file_path, GetPurpose());
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
		 ============================================================================
		*/
		class AudioFileStrategy : public IFileProcessStrategy {
		public:
			virtual FilePurpose GetPurpose() const override
			{
				return FilePurpose::FileExtract;
			}

			virtual bool Process(AI& ai, const std::string& file_path,
				nlohmann::json& out_messages, nlohmann::json& out_parts) override
			{
				std::string file_id = UploadAndGetId(ai, file_path, GetPurpose());
				if (file_id.empty())
				{
					return false;
				}

				std::string text = ExtractTextContent(ai.GetFileContent(file_id));
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
		 ============================================================================
		*/
		class FileStrategyFactory {
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
				{
					std::lock_guard<std::mutex> lock(m_mutex_custom);
					auto it = m_custom_strategies.find(file_type);
					if (it != m_custom_strategies.end())
					{
						return it->second;
					}
				}

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

		private:
			// C++17 inline static members guarantee a single definition for a header-only library
			inline static std::unordered_map<FileType, std::shared_ptr<IFileProcessStrategy>> m_custom_strategies;
			inline static std::mutex m_mutex_custom;
		};
	}

	/*
	 ============================================================================
	 Function: FilesToMessages
	 Description: Convert multiple files into a messages array ready for chat (implementation of
				 the AI class member function). Internally, a processing strategy is selected for
				 each file through the strategy factory
	 Parameters:
		 - const std::vector<std::string>& file_paths: Array of local file paths
	 Return: Returns the messages array
	 ============================================================================
	*/
	inline nlohmann::json AI::FilesToMessages(const std::vector<std::string>& file_paths)
	{
		nlohmann::json messages = nlohmann::json::array();
		nlohmann::json media_parts = nlohmann::json::array();

		for (const std::string& file_path : file_paths)
		{
			FileOperator::FileType file_type = FileOperator::FileTypeDetector::DetectFileType(file_path);
			std::shared_ptr<FileOperator::IFileProcessStrategy> strategy =
				FileOperator::FileStrategyFactory::Create(file_type);

			if (strategy == nullptr || !strategy->Process(*this, file_path, messages, media_parts))
			{
				DoErrorThrow("AI: failed to process file: " + file_path);
			}
		}

		// Media content (images/videos) is merged into the content parts of a single user message
		if (!media_parts.empty())
		{
			messages.push_back({ {"role", "user"}, {"content", media_parts} });
		}

		return messages;
	}

}

#define ALL_AI_TOOL_MESSAGE_ROLE_USER		(ALL_AI::JsonOperatorTools::Role::User)
#define ALL_AI_TOOL_MESSAGE_ROLE_ASSISTANT	(ALL_AI::JsonOperatorTools::Role::Assistant)
#define ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM		(ALL_AI::JsonOperatorTools::Role::System)

#endif