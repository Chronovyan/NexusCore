"""
AI Coding Assistant Test File
-----------------------------
This file contains various code patterns to test the AI assistant's capabilities.
Try the following:
1. Place cursor after 'def fibonacci(' and wait for autocomplete
2. Hover over functions to see documentation
3. Try writing a new function and see if the AI suggests completions
"""

class DataProcessor:
    """A class to process and analyze data."""
    
    def __init__(self, data=None):
        """Initialize with optional data."""
        self.data = data or []
    
    def add_data(self, value):
        """Add a single data point."""
        self.data.append(value)
    
    def calculate_stats(self):
        """Calculate basic statistics."""
        if not self.data:
            return None
            
        return {
            'sum': sum(self.data),
            'avg': sum(self.data) / len(self.data),
            'min': min(self.data),
            'max': max(self.data)
        }

# Test the fibonacci function
def fibonacci(n):
    """Calculate the nth Fibonacci number."""
    if n <= 0:
        return "Please enter a positive integer"
    elif n == 1 or n == 2:
        return 1
    
    a, b = 1, 1
    for _ in range(3, n + 1):
        a, b = b, a + b
    
    return b

# TODO: Implement a function to sort a list of dictionaries by a specific key
# Start typing 'def sort_di' and see if the AI can help complete it

def sort_di
