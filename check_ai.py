import sys
import os

def check_ai_setup():
    print("=== AI Setup Check ===")
    
    # Check if model file exists
    model_path = os.path.expandvars(r'%APPDATA%\\AI-First-TextEditor\\config.json')
    if os.path.exists(model_path):
        print(f"[OK] Config file found at: {model_path}")
    else:
        print(f"[ERROR] Config file not found at: {model_path}")
    
    # Check if we can import the provider
    try:
        from LlamaProvider import LlamaProvider
        print("[OK] LlamaProvider import successful")
    except ImportError as e:
        print(f"[ERROR] Failed to import LlamaProvider: {e}")
        print("This usually means the module isn't in your Python path.")
    
    print("\nNext steps:")
    print("1. Open ai_test.py in your editor")
    print("2. Try typing code to see AI completions")
    print("3. Hover over functions to see documentation")

if __name__ == "__main__":
    check_ai_setup()
