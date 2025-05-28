# Beta Evolve
A dual-AI collaborative programming system that uses two specialized AI agents to iteratively solve complex programming problems. Beta Evolve combines the speed of rapid prototyping with the depth of analytical reasoning to generate high-quality, robust C code solutions.

It is designed to both create files from scratch and evolve existing code to solve real-world problems, mathematical challenges, or algorithmic tasks, with a focus on error handling, optimization, and comprehensive testing.

## Overview
Beta Evolve employs a novel two-agent approach to code generation:
- **Design Agent** 🏃: Focuses on rapid prototyping and core functionality, quickly generating working code that compiles and addresses the main problem requirements.
- **Reasoning Agent** 🧠: Analyzes and refines the code, focusing on error handling, optimization, edge cases, memory safety, and comprehensive testing.

The agents collaborate through multiple iterations, with each cycle improving code quality, fixing errors, and enhancing robustness until a production-ready solution is achieved.

## Features
- **Dual-AI Architecture**: Two specialized agents with complementary strengths
- **Code Evolution Mode**: Evolve specific regions of existing code files using evolution markers
- **Iterative Refinement**: Continuous improvement through multiple collaboration cycles
- **Comprehensive Testing**: Automatic code compilation, execution, and validation
- **Custom Test Integration**: Configurable test commands for evolution validation
- **Flexible AI Backend**: Supports OpenAI API, local models, or custom endpoints
- **Local AI Server**: Built-in Flask server for running HuggingFace models locally
- **Configurable Output**: Multiple verbosity levels and colored terminal output
- **Error Recovery**: Automatic error detection and fixing with intelligent retry logic
- **Solution Persistence**: Saves final solutions with timestamps and problem descriptions
- **Cross-Platform Support**: Works on Windows, macOS, and Linux

## Quick Start

### 1. Installation
```bash
# Clone the repository
git clone https://github.com/zanderlewis/beta-evolve.git
cd beta-evolve

# Install system dependencies (macOS)
brew install libcjson pkg-config

# Install Python dependencies for local AI server (optional)
pip install -r requirements.txt

# Build the project
make
```

### 2. Configuration
Copy the example configuration file and customize it:
```bash
cp config.toml.example config.toml
```

Edit `config.toml` with your preferred settings:
```toml
# API Configuration
fast_model_api_key = "your-api-key"
reasoning_model_api_key = "your-api-key"
fast_model_endpoint = "https://api.openai.com/v1/chat/completions"
reasoning_model_endpoint = "https://api.openai.com/v1/chat/completions"
fast_model_name = "gpt-3.5-turbo"
reasoning_model_name = "gpt-4"
iterations = 10
```

### 3. Basic Usage
```bash
./beta_evolve --help  # Show help message
```

## Usage Examples

### Command Line Problem
```bash
./beta_evolve --problem "Create a thread-safe queue data structure in C with producer-consumer functionality"
```

### Using Problem Files
```bash
# Create a problem file
echo "Implement a NASA-grade binary search with error handling" > my_problem.prompt

# Run with the problem file
./beta_evolve --prompt-file my_problem.prompt --iterations 15
```

### Verbose Output
```bash
./beta_evolve --verbose --problem "Optimize matrix multiplication for large datasets"
```

### Debug Mode
```bash
./beta_evolve --debug --config custom_config.toml --prompt-file complex_problem.prompt
```

## Local AI Server
Beta Evolve includes a Flask server for running HuggingFace models locally:
```bash
# Start local server with a model
python server.py microsoft/DialoGPT-medium --port 5000

# Use custom temperature
python server.py google/flan-t5-large --temperature 0.7 --port 8080

# Configure Beta Evolve to use local server
# In config.toml:
fast_model_endpoint = "http://localhost:5000/v1/chat/completions"
reasoning_model_endpoint = "http://localhost:8080/v1/chat/completions"
fast_model_api_key = ""  # No API key needed for local server
```

## Configuration Options

### Core Settings
- `iterations`: Number of collaboration cycles (default: 10)
- `max_response_size`: Maximum AI response size in bytes
- `max_prompt_size`: Maximum prompt size in bytes
- `max_conversation_turns`: Maximum conversation history length
- `max_code_size`: Maximum generated code size

### AI Model Configuration
- `fast_model_endpoint`: API endpoint for the fast agent
- `reasoning_model_endpoint`: API endpoint for the reasoning agent
- `fast_model_name`: Model name for fast agent
- `reasoning_model_name`: Model name for reasoning agent
- API keys for authenticated services

### Evolution Mode Settings
- `enable_evolution`: Enable code evolution mode (true/false)
- `evolution_file_path`: Path to the C file containing evolution markers
- `test_command`: Custom command to test evolved code (use `{file}` placeholder)

### Execution Options
- `args`: Additional compilation/execution arguments
- `problem_prompt_file`: Default problem file to load
- Verbosity and output formatting controls

## Output and Logging
Beta Evolve provides multiple output modes:
- **Normal**: Shows iteration progress and error status
- **Verbose** (`--verbose`): Includes AI responses and detailed information
- **Debug** (`--debug`): Shows API calls, JSON payloads, and internal operations
- **Quiet** (`--quiet`): Minimal output only

All interactions are logged to `beta-evolve.log` for debugging and analysis.

## Architecture

### Dual-Agent Workflow
1. **Problem Analysis**: Both agents receive the problem description or existing code file
2. **Fast Agent Turn**: Generates rapid prototype focusing on core functionality
3. **Code Testing**: Automatic compilation and execution validation
4. **Reasoning Agent Turn**: Analyzes and refines the code
5. **Error Detection**: Identifies compilation, runtime, or logical errors
6. **Iterative Improvement**: Repeat until error-free solution is achieved
7. **Solution Persistence**: Save final working solution with metadata

### Evolution Mode Workflow
When evolution mode is enabled:
1. **Code Parsing**: Parse existing code file for evolution markers (`// BETA EVOLVE START/END`)
2. **Region Identification**: Identify specific code regions to evolve
3. **Targeted Evolution**: Apply AI improvements only to marked regions
4. **Custom Testing**: Run user-defined test commands for validation
5. **Generation Tracking**: Track evolution generations and fitness scores
6. **Selective Improvement**: Preserve working code while evolving specific areas

## Advanced Features

### Code Evolution System
Beta Evolve can evolve existing code files using special markers:
- **Evolution Markers**: Use `// BETA EVOLVE START` and `// BETA EVOLVE END` to mark evolvable regions
- **Selective Evolution**: Only marked code regions are modified, preserving the rest
- **Custom Testing**: Configure test commands to validate evolved code
- **Generation Tracking**: Monitor evolution progress and fitness scores
- **Rollback Safety**: Original code is preserved while new versions are generated

### Error Recovery
Beta Evolve automatically detects and recovers from:
- Compilation errors (syntax, missing includes, type mismatches)
- Runtime errors (segfaults, memory leaks, infinite loops)
- Logic errors (incorrect output, failed test cases)

### Testing Integration
- Automatic code compilation with strict warnings
- Runtime execution with timeout protection
- Memory leak detection
- Comprehensive test case validation
- Custom test command support for evolution mode

### Extensibility
- Pluggable AI backends (OpenAI, local models, custom APIs)
- Configurable agent prompts and behavior
- Custom testing frameworks
- Problem template system
- Evolution marker customization

## Contributing
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Submit a pull request

## Dependencies
### System Requirements
- GCC compiler with C99 support
- libcjson library (install via `brew install libcjson` on macOS)
- pkg-config (install via `brew install pkg-config` on macOS)
- Make build system

### Optional Dependencies
- Python 3.7+ (for local AI server)
- HuggingFace Transformers (for local models): `pip install transformers torch`
- Flask (for web interface): `pip install flask`
