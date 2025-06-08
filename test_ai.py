import json
import os
import sys
from pathlib import Path

def test_ai_provider():
    # Set console output encoding to UTF-8
    if sys.stdout.encoding != 'utf-8':
        import io
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    
    # Load configuration
    config_path = os.path.expandvars(r'%APPDATA%\\AI-First-TextEditor\\config.json')
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            config = json.load(f)
        print("[OK] Configuration loaded successfully")
    except Exception as e:
        print(f"[ERROR] Failed to load config: {e}")
        return
    print(f"AI Provider: {config.get('ai_provider', 'Not set')}")
    
    # Check model file exists
    model_path = config.get('llama', {}).get('model_path', '')
    if os.path.exists(model_path):
        print(f"[OK] Model file found at: {model_path}")
        print(f"    Size: {os.path.getsize(model_path) / (1024*1024):.2f} MB")
    else:
        print(f"[ERROR] Model file not found at: {model_path}")
        return
    
    # Test if we can import the provider
    try:
        from LlamaProvider import LlamaProvider
        print("[OK] LlamaProvider imported successfully")
        
        # Try to initialize the provider
        provider = LlamaProvider(os.path.dirname(model_path))
        options = {
            'model_path': model_path,
            'template_id': 'code_completion',
            'context_size': config.get('llama', {}).get('context_size', 2048),
            'threads': config.get('llama', {}).get('threads', 4)
        }
        
        if provider.initialize(options):
            print("[OK] LlamaProvider initialized successfully")
            
            # Test a simple completion
            test_prompt = "def hello_world():"
            print(f"\nTesting completion for: {test_prompt}")
            
            # This is a simplified test - actual implementation may vary
            try:
                response = provider.send_completion_request([{"role": "user", "content": test_prompt}])
                print("[OK] Completion test successful!")
                print("Response preview:", str(response)[:100] + "...")
            except Exception as e:
                print(f"[WARNING] Completion test failed: {str(e)}")
        else:
            print("[ERROR] Failed to initialize LlamaProvider")
            
    except ImportError as e:
        print(f"[ERROR] Failed to import LlamaProvider: {e}")
        print("Make sure you're running this from the correct directory and the module is in your PYTHONPATH")
    except Exception as e:
        print(f"[ERROR] An error occurred: {e}")

if __name__ == "__main__":
    print("[TEST] Testing AI Provider Configuration")
    print("=" * 50)
    test_ai_provider()
