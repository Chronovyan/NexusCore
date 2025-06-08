# AI-First TextEditor Tutorials

This directory contains JSON files that define interactive tutorials for the AI-First TextEditor application. These tutorials provide step-by-step guidance for users to learn various aspects of the application.

## Tutorial Structure

Each tutorial is defined in a JSON file with the following structure:

```json
{
  "id": "unique_tutorial_id",
  "title": "Tutorial Title",
  "description": "Brief description of the tutorial",
  "type": 0,                                  // 0 = Beginner, 1 = Intermediate, 2 = Advanced
  "difficulty": 1,                           // 1-5 scale of difficulty
  "estimatedTime": "15 min",                 // Estimated completion time
  "tags": ["tag1", "tag2", "tag3"],          // Searchable tags
  "steps": [
    {
      "id": "step1",
      "title": "Step 1 Title",
      "description": "Step 1 description and instructions",
      "required_actions": ["action1", "action2"] // Actions required to complete this step
    },
    // More steps...
  ]
}
```

## Creating a New Tutorial

To create a new tutorial:

1. Create a new JSON file with a descriptive name (e.g., `feature_name_tutorial.json`)
2. Define the tutorial structure following the format above
3. Add steps that guide the user through a specific feature or workflow
4. Specify required actions for each step (if applicable)

### Tutorial Types

- **0 (Beginner)**: Suitable for new users, covering basic features
- **1 (Intermediate)**: For users familiar with the basics, introducing more advanced features
- **2 (Advanced)**: For experienced users, covering complex features and workflows

### Difficulty Scale

- **1**: Very easy, suitable for complete beginners
- **2**: Easy, requires basic understanding of the application
- **3**: Moderate, requires some familiarity with related concepts
- **4**: Challenging, involves multiple features or complex workflows
- **5**: Very challenging, involves advanced concepts or techniques

### Required Actions

Required actions are identifiers for actions that need to be performed to complete a step. The application will track these actions and only allow progression when all required actions for a step are completed.

Common action types include:
- `"settings_menu_click"`: User clicked the Settings menu
- `"api_key_save"`: User saved an API key
- `"model_selection_dialog"`: User opened the model selection dialog
- `"change_model"`: User changed the active model
- `"send_message"`: User sent a message in the chat
- `"select_file"`: User selected a file in the project panel

## Markdown in Descriptions

Tutorial descriptions and step descriptions support Markdown formatting, including:
- **Bold text** with `**text**`
- *Italic text* with `*text*`
- Lists with `1.`, `2.`, etc. or `-` for bullet points
- Code blocks with triple backticks
- Line breaks with `\n`

## Example

See the existing tutorials like `getting_started.json` for a complete example of how to structure a tutorial.

## Testing Your Tutorial

After creating a tutorial:

1. Place the JSON file in this directory
2. Launch the AI-First TextEditor application
3. Click "Tutorials" in the left panel
4. Find your tutorial in the browser and test it

## Best Practices

- Keep steps concise and focused on a single task or concept
- Provide clear instructions with visual cues when possible
- Use a consistent writing style across steps
- Include screenshots or code examples when helpful
- Test the tutorial thoroughly to ensure all steps can be completed
- Consider the logical progression of concepts in multi-step tutorials 