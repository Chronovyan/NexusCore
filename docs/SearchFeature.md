# Using Search and Replace

This guide explains how to use the search and replace functionality in the text editor to efficiently find and modify text.

## Key Capabilities

*   **Find Text**: Quickly locate specific words or phrases.
*   **Navigate Matches**: Easily jump from one match to the next.
*   **Case Sensitivity**: Choose whether your search respects letter casing (e.g., "Text" vs "text").
*   **Visual Feedback**: Found text is automatically highlighted (selected).
*   **Replace Single Match**: Change the currently highlighted match.
*   **Replace All Matches**: Globally change all occurrences of a term.
*   **Undo Support**: All replace operations can be undone.

## Commands

| Command                                      | Description                                                        |
| -------------------------------------------- | ------------------------------------------------------------------ |
| `search <text>` or `find <text>`             | Search for `<text>`. The first match will be highlighted.          |
| `searchnext` or `findnext`                 | Move to and highlight the next occurrence of the current search term. |
| `replace <search_term> <replacement_text>` | Find `<search_term>` and replace its *next* occurrence with `<replacement_text>`. If a search is already active, it replaces the current/next highlighted match. |
| `replaceall <search_term> <replacement_text>`| Replace all occurrences of `<search_term>` with `<replacement_text>` in the entire document.|

*Note: For case-sensitive search with commands that take `<search_term>`, you may need a separate command or a global setting if not implicitly handled. Refer to `help search` or editor settings if available. By default, search is often case-insensitive unless specified.* 

## Example Workflow

Let's say your document contains:

```
Line 0: a small cat sat on the mat.
Line 1: the cat was small and gray.
Line 2: cats and small dogs are friends.
```

1.  **Start a search:**
    ```
    > search small
    ```
    *Effect:* The editor highlights "small" on Line 0. Cursor might be positioned after it.

2.  **Find the next match:**
    ```
    > searchnext
    ```
    *Effect:* The editor highlights "small" on Line 1.

3.  **Replace the current match:**
    Suppose you want to change the "small" on Line 1 to "tiny". With "small" on Line 1 highlighted:
    ```
    > replace small tiny 
    ``` 
    *Effect:* Line 1 becomes: `the cat was tiny and gray.` The next "small" (on Line 2) might be highlighted.
    *Buffer might look like:*
    ```
    Line 0: a small cat sat on the mat.
    Line 1: the cat was tiny and gray.
    Line 2: cats and small dogs are friends.
    ```

4.  **Replace all remaining instances:**
    To change all remaining "small" to "little":
    ```
    > replaceall small little
    ```
    *Effect:* All instances of "small" are changed to "little".
    *Buffer will now be:*
    ```
    Line 0: a little cat sat on the mat.
    Line 1: the cat was tiny and gray.
    Line 2: cats and little dogs are friends.
    ```

(Use the `undo` command if you need to revert any replacements.) 