"""
Simple AI Test Script
---------------------
This script demonstrates how to use the AI features in the editor.
Just type code and see the AI suggestions as you type!
"""

# Try typing a function definition and see if you get suggestions
def calculate_average(numbers):
    """Calculate the average of a list of numbers."""
    if not numbers:
        return 0
    return sum(numbers) / len(numbers)

# Try typing 'data = {' and see if the AI suggests dictionary keys
data = {
    'name': 'Test',
    'value': 42,
    # Try adding more keys here
}

# Try writing a class and see the AI help with method suggestions
class TestClass:
    def __init__(self, name):
        self.name = name
    
    # Try typing 'def get_' and see if the AI suggests method names
    def get_name(self):
        return self.name
    
    # Try adding a new method here
