# LlamaProvider: Using Local LLama Models in AI-First TextEditor

This guide explains how to use the LlamaProvider to integrate local LLama models with the AI-First TextEditor.

## Overview

The LlamaProvider enables you to use local LLama models within the editor, providing several advantages:

- **Privacy**: Your data stays on your device
- **Offline Use**: No internet connection required
- **No Usage Costs**: Run as many queries as you want
- **Customizability**: Use fine-tuned models for specific domains

## Supported Models

The LlamaProvider supports various LLama model formats:

- LLama 2 models (7B, 13B, 70B variants)
- LLama-based instruct models (Alpaca, Vicuna, etc.)
- GGUF format models (.gguf extension)
- GGML format models (.ggml extension, legacy support)

## Setup Instructions

### 1. Obtain LLama Models

Download compatible models from one of these sources:

- [HuggingFace](https://huggingface.co/models?search=llama)
- [The Bloke's GGUF models](https://huggingface.co/TheBloke)
- [LLama 2 Official Website](https://ai.meta.com/llama/) (requires application approval)

Recommended starter models:
- `llama-2-7b-chat.Q4_K_M.gguf` (2.9GB, balanced performance)
- `llama-2-13b-chat.Q4_K_M.gguf` (5.3GB, better quality)
- `codellama-7b-instruct.Q4_K_M.gguf` (3.1GB, optimized for code)

### 2. Place Models in a Directory

Create a dedicated directory for your models, for example:
- Windows: `C:\Users\YourUsername\AIModels\llama\`
- macOS/Linux: `/home/YourUsername/AIModels/llama/`

### 3. Configure the Editor

#### Through Settings Dialog:

1. Open the editor settings (File > Settings)
2. Navigate to AI > Models
3. Click "Add Local Model"
4. Browse to your model directory
5. Select the model file you want to use
6. Click "Add Model"

#### Through Configuration File:

Add the following to your `config.json` file:

```json
{
  "ai": {
    "providers": {
      "llama": {
        "enabled": true,
        "model_path": "C:/Users/YourUsername/AIModels/llama/",
        "default_model": "llama-2-7b-chat.Q4_K_M.gguf"
      }
    }
  }
}
```

### 4. Switching Between Models

1. Click on the AI model indicator in the status bar
2. Select "LLama" from the provider dropdown
3. Choose your desired model from the model dropdown

Alternatively, use the command palette (Ctrl+Shift+P) and search for "AI: Change Model".

## Advanced Configuration

### Memory Usage

You can configure memory usage for the LLama models in the settings:

```json
{
  "ai": {
    "providers": {
      "llama": {
        "memory_limit": "4GB",
        "context_size": 4096
      }
    }
  }
}
```

### Custom Prompt Templates

Create custom prompt templates for different model types:

```json
{
  "ai": {
    "providers": {
      "llama": {
        "prompt_templates": {
          "alpaca": "<system>\n{system_message}\n</system>\n\n<user>\n{user_message}\n</user>\n\n<assistant>\n",
          "llama2": "[INST] {user_message} [/INST]"
        }
      }
    }
  }
}
```

## Troubleshooting

### Model Loading Issues

If your model fails to load:

1. Check if the model file exists and is not corrupted
2. Ensure you have sufficient RAM for the model size
3. Try a smaller quantized model if memory is limited
4. Check the logs for specific error messages

### Performance Issues

If the model is running slowly:

1. Use a smaller model (7B instead of 13B)
2. Try a more aggressively quantized version (Q4 instead of Q8)
3. Reduce the context size in settings
4. Close other memory-intensive applications

### Output Quality Issues

If you're getting poor quality outputs:

1. Try a larger model if your hardware supports it
2. Use models specifically fine-tuned for your use case
3. Experiment with different prompt templates
4. Consider more precise quantization formats (Q8 instead of Q4)

## Technical Details

The LlamaProvider integrates with the editor through the IAIProvider interface, providing:

- Model discovery and loading
- Chat message processing
- Tool call detection and parsing
- Context management
- Thread-safe operations

Model inference is performed in a background thread to keep the UI responsive.

## Future Enhancements

Planned improvements for the LlamaProvider include:

- GPU acceleration support
- Streaming responses
- Advanced quantization options
- Custom model fine-tuning integration
- Improved tool calling capabilities

## Contributing

If you'd like to contribute to improving the LlamaProvider:

1. Submit issues on GitHub for bugs or feature requests
2. Contribute optimizations for specific hardware
3. Share prompt templates that work well with different models
4. Help with testing new model formats as they become available 