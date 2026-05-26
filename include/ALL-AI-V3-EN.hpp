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
*
* ====================================================================================================
*
*   Developer's notes:
*   1. The developer is not an AI specialist, and my abilities are limited. Thank you for your understanding.
*   2. The developer is currently seeking a job (major: Computer Science and Technology). If you would like to offer an opportunity, please contact me via the email below.
*   3. Open-source license: MIT
*
*   Online documentation: https://ai-cpp-docsify.cpluscottage.top/
*   Personal blog: https://blog.wang-sz.cn
*   Feedback / updates / contact email: about@wang-sz.cn
*
*   If this library helps you, please consider giving it a star. Your support is my greatest motivation!
*
*   ！！！Translation from OpenAI (GPT-4o-mini | Kimi-k2.5)！！！
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

	// HTTP method enumeration. Currently only libcurl-based sessions are supported.
	enum class HttpMethod {
		POST,
		GET
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

		void SetThrowErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_function);

	protected:
		ALL_AI_ErrorThrow m_ErrorThrow = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW;
		std::function<void(const std::string_view& message)> m_callback_function;
	};

	/*
	============================================================================
	Function: SetThrowErrorCallbackFunction
	Description: Set the callback function used for error reporting
	Parameters:
		- std::function<void(const std::string_view& message)> callback_function: A callback that receives error messages
	Return: No return value
	============================================================================
	*/
	void ThrowError::SetThrowErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_function)
	{
		if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
			callback_function != nullptr)
		{
			this->m_callback_function = callback_function;
		}
		return;
	}

	// Request builder strategy
	class IRequestBuilderStrategy : public ThrowError {
	public:
		virtual ~IRequestBuilderStrategy() = default;
		virtual nlohmann::json GetBuilder() = 0;
		virtual void ClearBuilder() = 0;
		virtual nlohmann::json GetEmptyBuilder() = 0;
	};

	// Response parser strategy
	class IResponseParserStrategy : public ThrowError {
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
			virtual nlohmann::json GetBuilder() override
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
			bool SetValue(_T_Value _value, Args... _keys);

			// Appends to an array (creates array if path doesn't exist, fails if exists but is not an array)
			template <typename _T_Value, typename... Args>
			bool AppendToArray(_T_Value _value, Args... _keys);

			// Inserts or replaces a value at a specific array index
			template <typename _T_Value, typename... Args>
			bool SetArrayValue(_T_Value _value, size_t index, Args... _keys);

			// Gets array length (returns 0 if path doesn't exist, -1 if not an array)
			template <typename... Args>
			int GetArrayLength(Args... _keys);

			// Creates an empty array
			template <typename... Args>
			bool CreateArray(Args... _keys);

			// Creates an empty object
			template <typename... Args>
			bool CreateObject(Args... _keys);

		private:

			// Path element type: can be a string key or an array index
			using PathKey = std::variant<std::string, size_t, int>;

			// Terminates recursion
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path);

			// String key
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const std::string& _key);

			// Array index
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const char* _key);

			// Array index（size_t 或 int）
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, size_t _index);
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& path, int index);

			// Variadic parameter expansion
			template <typename T, typename... Rest>
			void BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... rest);

			// Converts variadic parameters to a path array
			template <typename... Args>
			std::vector<PathKey> BuildPath(Args&&... args);

			// Navigates to or creates a node by path (auto-creates intermediate objects/arrays)
			nlohmann::json* NavigateOrCreate(nlohmann::json& root, const std::vector<PathKey>& path, bool createMissing = true);

			// Navigates to a node by path (read-only, no creation)
			nlohmann::json* Navigate(nlohmann::json& root, const std::vector<PathKey>& path);

			// Sets a JSON field value: recursion termination layer
			template <typename T>
			bool _setValue(nlohmann::json& _json, T&& val, const std::string& key);

			// Sets a JSON field value: recursion intermediate layer
			template <typename T, typename... Args>
			bool _setValue(nlohmann::json& _json, T&& val, const std::string& first, Args&&... rest);

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
		bool JsonRequestBuilder::SetValue(_T_Value _value, Args... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			// Uses new NavigateOrCreate to replace original recursive _setValue
			std::vector<PathKey> path = BuildPath(_keys...);
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
				(*parent)[std::get<std::string>(lastKey)] = _value;
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
				(*parent)[index] = _value;
			}
			return true;
		}

		/*
		 ============================================================================
		 Function: AppendToArray
		 Description: Appends an element to the end of a JSON array
		 Parameters:
			 - _T_Value: The value to be set
			 - _Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
		 ============================================================================
		*/
		template<typename _T_Value, typename ...Args>
		inline bool JsonRequestBuilder::AppendToArray(_T_Value _value, Args ..._keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(_keys...);
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
			node->push_back(_value);
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
		bool JsonRequestBuilder::SetArrayValue(_T_Value _value, size_t index, Args... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(_keys...);
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
				node->push_back(_value);
			}
			else
			{
				(*node)[index] = _value;
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
		int JsonRequestBuilder::GetArrayLength(Args... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(_keys...);
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
		 Function: CreateArray
		 Description: Creates a JSON array
		 Parameters:
			 - Args...: Variadic parameters, must be strings, used as JSON field indices
		 Return: Returns true on success, false otherwise
		 ============================================================================
		*/
		template <typename... Args>
		bool JsonRequestBuilder::CreateArray(Args... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(_keys...);
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
		bool JsonRequestBuilder::CreateObject(Args... _keys)
		{
			std::lock_guard<std::mutex> lock(this->m_mutex_request);

			std::vector<JsonRequestBuilder::PathKey> path = BuildPath(_keys...);
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
		 Function: BuildPathImpl
		 Description: Builds path - Recursion termination
		 Parameters:
			 - std::vector<JsonRequestBuilder::PathKey>& path: The path
		 Return: None
		 ============================================================================
		*/
		void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path)
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
		void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const std::string& _key)
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
		void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, const char* _key)
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
		void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, size_t _index)
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
		void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, int _index)
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
		void JsonRequestBuilder::BuildPathImpl(std::vector<JsonRequestBuilder::PathKey>& _path, T&& _first, Rest&&... rest)
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
		std::vector<JsonRequestBuilder::PathKey> JsonRequestBuilder::BuildPath(Args&&... args)
		{
			std::vector<PathKey> path;
			BuildPathImpl(path, std::forward<Args>(args)...);
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
		nlohmann::json* JsonRequestBuilder::NavigateOrCreate(nlohmann::json& root, const std::vector<PathKey>& path, bool createMissing)
		{
			nlohmann::json* current = &root;

			for (const PathKey& key : path)
			{
				std::visit([&](auto&& k) {
					using T = std::decay_t<decltype(k)>;

					if constexpr (std::is_same_v<T, std::string>)
					{
						// Object key access
						if (!current->contains(k))
						{
							if (!createMissing)
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
						// Array index访问
						if (!current->is_array())
						{
							if (!createMissing || !current->is_null())
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
							if (!createMissing)
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
		nlohmann::json* JsonRequestBuilder::Navigate(nlohmann::json& root, const std::vector<PathKey>& path)
		{
			nlohmann::json* current = &root;

			for (const auto& key : path)
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
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "JsonParser::Parse: " << err << std::endl;
						return;
					}
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
		_T_Type JsonResponceParser::GetValue(_Keys... _keys)
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
		_T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _Key&& key)
		{
			if constexpr (std::is_integral_v<std::decay_t<_Key>>)
			{
				// Array index
				if (!_json.is_array() || key < 0 || key >= _json.size())
				{
					if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Array index out of bounds: " + std::to_string(key));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Array index out of bounds: " + std::to_string(key));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "Array index out of bounds: " << key << std::endl;
					}
					return _T_Type{};
				}
				return _json.at(key);
			}
			else
			{
				// Object key
				if (!_json.contains(key))
				{
					if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Key not found: " + std::string(key));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{
						this->m_callback_function("Key not found: " + std::string(key));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "Key not found: " << key << std::endl;
					}
					return _T_Type{};
				}
				return _json.at(key);
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
		_T_Type JsonResponceParser::_getValue(const nlohmann::json& _json, _First&& first, Args... rest)
		{
			nlohmann::json next_json = nullptr;

			// Determine whether `first` is an array index or an object key and handle access errors accordingly
			if constexpr (std::is_integral_v<std::decay_t<_First>>)
			{
				// Array index
				if (!_json.is_array() || first < 0 || first >= _json.size())
				{
					if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Array index out of bounds");
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Array index out of bounds: " + std::to_string(first));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "Array index out of bounds: " << first << std::endl;
					}
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
					if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::out_of_range("Key not found: " + std::string(first));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
						this->m_callback_function != nullptr)
					{

						this->m_callback_function("Key not found: " + std::string(first));
					}
					else if (this->m_ErrorThrow == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "Key not found: " << first << std::endl;
					}
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

	// Abstract HTTP transport interface that defines how HTTP requests are sent
	class IHttpTransport {
	public:
		IHttpTransport() = default;
		virtual ~IHttpTransport() = default;
		// Initialize the HTTP transport interface with the URL, API key, and error handling mode
		virtual bool Initialize(const std::string& url, const std::string& api_key, const ALL_AI_ErrorThrow all_ai_error_throw) = 0;
		// Send an HTTP request
		virtual nlohmann::json SendRequest(HttpMethod method, const nlohmann::json request_json) = 0;

		// Set the error callback function
		virtual void SetErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_func) = 0;
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
				if (this->m_curl != nullptr)
				{
					// Clean up libcurl
					curl_easy_cleanup(this->m_curl);
				}
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
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("CurlHttpTransport: url or api_key is empty");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("CurlHttpTransport: url or api_key is empty");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "CurlHttpTransport: url or api_key is empty" << std::endl;
					}
					return false;
				}
				this->m_url = url;
				this->m_key = api_key;
				this->m_error_throw = all_ai_error_throw;

				// Initialize libcurl
				this->m_curl = curl_easy_init();
				if (!this->m_curl)
				{
					// If initialization fails, handle the error according to the selected error mode
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("CurlHttpTransport: curl_easy_init failed");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("CurlHttpTransport: curl_easy_init failed");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "CurlHttpTransport: curl_easy_init failed" << std::endl;
					}
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
				if (CheckStringEmpty(this->m_key))
				{
					return nlohmann::json{};
				}
				std::string authHeader = "Authorization: Bearer " + this->m_key;
				headers = curl_slist_append(headers, authHeader.c_str());
				headers = curl_slist_append(headers, "Content-Type: application/json");
				curl_easy_setopt(this->m_curl, CURLOPT_HTTPHEADER, headers);

				// Clear any residual request flags
				// clear flags
				curl_easy_setopt(this->m_curl, CURLOPT_POST, 0L);
				curl_easy_setopt(this->m_curl, CURLOPT_POSTFIELDS, nullptr);
				curl_easy_setopt(this->m_curl, CURLOPT_NOBODY, 0L);

				// Set the request payload
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

				// Execute the request
				CURLcode res = curl_easy_perform(this->m_curl);
				if (res != CURLE_OK)
				{
					std::string error_message = "curl_easy_perform failed: " + std::string(curl_easy_strerror(res));
					curl_slist_free_all(headers); // Ensure we free headers
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << error_message << std::endl;
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
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function(error_message);
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << error_message << std::endl;
					}
					return nlohmann::json{};
				}

				// Check if response is empty
				if (CheckStringEmpty(str_Buffer))
				{
					curl_slist_free_all(headers);
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
						if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
						{
							throw std::runtime_error(error_message);
						}
						else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
						{
							this->m_callback_function(error_message);
							return nlohmann::json{};
						}
						else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
						{
							std::cerr << error_message << std::endl;
							return nlohmann::json{};
						}
					}
				}

				// Clean up headers
				curl_slist_free_all(headers);
				return json_result;
			}
			
			/*
			 ============================================================================
			 Function: CheckStringEmpty
			 Description: Check whether the string is empty
			 Parameters:
			   - const std::string& str: The string to be checked
			 Return: Returns a boolean value indicating whether the string is empty or not
			 ============================================================================
			*/
			virtual bool CheckStringEmpty(const std::string& str)
			{
				if(str.empty())
				{
					if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
					{
						throw std::runtime_error("Empty string");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION)
					{
						this->m_callback_function("Empty string");
					}
					else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
					{
						std::cerr << "Empty string" << std::endl;
					}
					return true;
				}
				else
				{
					return false;
				}
			}
			/*
			 ============================================================================
			 Function: SetErrorCallbackFunction
			 Description: Set the error callback function
			 Parameters:
			   - std::function<void(const std::string_view& message)>: A callback function that receives error messages
			 Return: No return value
			 ============================================================================
			*/
			virtual void SetErrorCallbackFunction(std::function<void(const std::string_view& message)> callback_func) override
			{
				this->m_callback_function = callback_func;
			}

			/*
			 ============================================================================
			 Function: SetErrorThrow
			 Description: Set the error handling mode
			 Parameters:
			   - ALL_AI_ErrorThrow: The error handling mode
			 Return: No return value
			 ============================================================================
			*/
			virtual void SetErrorThrow(ALL_AI_ErrorThrow error_throw)
			{
				this->m_error_throw = error_throw;
			}

			/*
			 ============================================================================
			 Function: SetErrorThrowCallbackFunction
			 Description: Set the callback function used for error reporting
			 Parameters:
			   - std::function<void(const std::string_view& message)>: A callback function that receives error messages as a string view
			 Return: No return value
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
						int index = choice.value("index", 0);
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
			m_api_key(api_key),
			m_error_throw(all_ai_error_throw) {
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
		 Function: SetErrorThrowCallbackFunction
		 Description: Set the callback function used for error reporting
		 Parameters:
			 - std::function<void(const std::string_view& message)>: A callback function that receives error messages as a string view
		 Return: No return value
		 ============================================================================
		*/
		bool SetErrorThrowCallbackFunction(std::function<void(const std::string_view& message)> callback_func)
		{
			// Switch the error handling mode to callback-based reporting and store the callback
			if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
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
		 Function: SetErrorThrow
   		 Description: Set the error throwing method
   P	 Parameters:
    		 - ALL_AI_ErrorThrow: An enumeration value indicating the way of error throwing
   		 Return: No return value
		 ============================================================================
		*/
		void SetErrorThrow(ALL_AI_ErrorThrow error_throw)
		{
			this->m_error_throw = error_throw;
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
			if (this->m_initialized == true)
			{
				return false;
			}
			// Set the error handling mode for the builder and parser
			if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
				this->m_callback_function != nullptr)
			{
				// No extra lock is required here because InitAI does not run concurrently with SendRequest
				// (this is guaranteed by the user or by the m_initialized flag), and Builder/Parser already have internal locks
				this->m_builder.SetThrowErrorCallbackFunction(this->m_callback_function);
				this->m_parser.SetThrowErrorCallbackFunction(this->m_callback_function);
			}

			// Create configuration copies for initialization
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

			// Reinitialize the transport
			if (this->m_transport)
			{
				return this->m_transport->Initialize(this->m_url, this->m_api_key, this->m_error_throw);
			}
			return false;
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
				if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_EXCEPTION_THROWING)
				{
					throw std::runtime_error("AI: HTTP transport is not set");
				}
				else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_CALLBACK_FUNCTION &&
					this->m_callback_function != nullptr)
				{
					this->m_callback_function("AI: HTTP transport is not set");
				}
				else if (this->m_error_throw == ALL_AI_ErrorThrow::ALL_AI_PRINT_ERROR)
				{
					std::cerr << "AI: HTTP transport is not set" << std::endl;
				}
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
			return SendRequest(HttpMethod::POST, this->m_builder.GetBuilder());
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
			return this->m_builder.GetBuilder();
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

		std::string m_url;	// API - URL
		std::string m_api_key;	// API - Key

		ALL_AI_ErrorThrow m_error_throw = ALL_AI_ErrorThrow::ALL_AI_NO_ERROR_THROW;		// Error handling mode

		std::shared_ptr<IHttpTransport> m_transport;	// HTTP transport interface
		std::function<void(const std::string_view& message)> m_callback_function = nullptr;		// Error callback function

		std::mutex m_mutex_config;		// Configuration mutex (protects URL, Key, and Transport)
		std::mutex m_mutex_ai_init;		// AI initialization mutex

		JsonOperator::JsonRequestBuilder m_builder;
		JsonOperator::JsonResponceParser m_parser;
		JsonOperatorTools m_tools;

		bool m_initialized = false;	// Whether AI has been initialized
	};

}

#define ALL_AI_TOOL_MESSAGE_ROLE_USER		(ALL_AI::JsonOperatorTools::Role::User)
#define ALL_AI_TOOL_MESSAGE_ROLE_ASSISTANT	(ALL_AI::JsonOperatorTools::Role::Assistant)
#define ALL_AI_TOOL_MESSAGE_ROLE_SYSTEM		(ALL_AI::JsonOperatorTools::Role::System)

#endif