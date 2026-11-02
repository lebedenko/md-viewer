#pragma once

// Explicit public QMarkdown 1.18 allowlist; grouped aliases appear only once.
inline constexpr auto kReaderSettings = R"json([
  {
    "key": "options.stylePreset",
    "kind": "enum",
    "tab": 0,
    "label": "QMarkdown style preset",
    "minimum": 0,
    "maximum": 5,
    "defaultValue": 0,
    "choices": [
      "Automatic",
      "Legacy",
      "Polished light",
      "Polished dark",
      "Minimal light",
      "Minimal dark"
    ]
  },
  {
    "key": "options.syntaxPalette",
    "kind": "enum",
    "tab": 4,
    "label": "Syntax Palette",
    "minimum": 0,
    "maximum": 2,
    "defaultValue": 0,
    "choices": [
      "Automatic",
      "Light",
      "Dark"
    ]
  },
  {
    "key": "options.gfmEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "GitHub Flavored Markdown (GFM)",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": true
  },
  {
    "key": "options.extendedAutolinksEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Extended Autolinks Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.strikethroughEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Strikethrough Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.tablesEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Tables Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.tagFilterEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Tag Filter Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.taskListsEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Task Lists Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.taskIndicatorsReplaceBullets",
    "kind": "bool",
    "tab": 6,
    "label": "Task indicators replace bullets",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": true
  },
  {
    "key": "options.footnotesEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Footnotes Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.inlineMathEnabled",
    "kind": "bool",
    "tab": 6,
    "label": "Inline Math Enabled",
    "minimum": 0,
    "maximum": 1000,
    "defaultValue": false
  },
  {
    "key": "options.codeBlockHeightRatio",
    "kind": "number",
    "tab": 3,
    "label": "Code height / viewport ratio",
    "minimum": 0.01,
    "maximum": 10,
    "defaultValue": 0.5
  },
  {
    "key": "options.codeBlockMinimumLines",
    "kind": "int",
    "tab": 3,
    "label": "Code Block Minimum Lines",
    "minimum": 1,
    "maximum": 10000,
    "defaultValue": 8
  },
  {
    "key": "options.codeBlockMaximumLines",
    "kind": "int",
    "tab": 3,
    "label": "Code Block Maximum Lines",
    "minimum": 1,
    "maximum": 10000,
    "defaultValue": 30
  },
  {
    "key": "style.bodyFont",
    "kind": "font",
    "tab": 1,
    "label": "Body Font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.bodyColor",
    "kind": "color",
    "tab": 1,
    "label": "Body Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.blockSpacing",
    "kind": "number",
    "tab": 1,
    "label": "Block Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.listIndent",
    "kind": "number",
    "tab": 1,
    "label": "List Indent",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.quoteIndent",
    "kind": "number",
    "tab": 1,
    "label": "Quote Indent",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.quoteRuleColor",
    "kind": "color",
    "tab": 1,
    "label": "Quote Rule Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.quoteRuleThickness",
    "kind": "number",
    "tab": 1,
    "label": "Quote Rule Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.linkColor",
    "kind": "color",
    "tab": 1,
    "label": "Link Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.linkUnderline",
    "kind": "bool",
    "tab": 1,
    "label": "Link Underline",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.thematicBreakColor",
    "kind": "color",
    "tab": 1,
    "label": "Thematic Break Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.thematicBreakThickness",
    "kind": "number",
    "tab": 1,
    "label": "Thematic Break Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.selectionBackgroundColor",
    "kind": "color",
    "tab": 1,
    "label": "Selection Background Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.lineNumberColor",
    "kind": "color",
    "tab": 3,
    "label": "Code Block / line Number Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.font",
    "kind": "font",
    "tab": 3,
    "label": "Code Block / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.color",
    "kind": "color",
    "tab": 3,
    "label": "Code Block / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.backgroundColor",
    "kind": "color",
    "tab": 3,
    "label": "Code Block / background Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.borderColor",
    "kind": "color",
    "tab": 3,
    "label": "Code Block / border Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.borderWidth",
    "kind": "number",
    "tab": 3,
    "label": "Code Block / border Width",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.cornerRadius",
    "kind": "number",
    "tab": 3,
    "label": "Code Block / corner Radius",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.horizontalPadding",
    "kind": "number",
    "tab": 3,
    "label": "Code Block / horizontal Padding",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.codeBlock.verticalPadding",
    "kind": "number",
    "tab": 3,
    "label": "Code Block / vertical Padding",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.inlineCode.font",
    "kind": "font",
    "tab": 3,
    "label": "Inline Code / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.inlineCode.foreground",
    "kind": "color",
    "tab": 3,
    "label": "Inline Code / foreground",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.inlineCode.backgroundColor",
    "kind": "color",
    "tab": 3,
    "label": "Inline Code / background Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.inlineCode.cornerRadius",
    "kind": "number",
    "tab": 3,
    "label": "Inline Code / corner Radius",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.inlineCode.horizontalPadding",
    "kind": "number",
    "tab": 3,
    "label": "Inline Code / horizontal Padding",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.inlineCode.verticalPadding",
    "kind": "number",
    "tab": 3,
    "label": "Inline Code / vertical Padding",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.headerForeground",
    "kind": "color",
    "tab": 5,
    "label": "Table / header Foreground",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.headerBackground",
    "kind": "color",
    "tab": 5,
    "label": "Table / header Background",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.bodyBackground",
    "kind": "color",
    "tab": 5,
    "label": "Table / body Background",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.borderMode",
    "kind": "enum",
    "tab": 5,
    "label": "Table / border Mode",
    "minimum": 0,
    "maximum": 3,
    "choices": [
      "None",
      "Grid",
      "Rows",
      "Header only"
    ]
  },
  {
    "key": "style.table.alternateRowBackground",
    "kind": "color",
    "tab": 5,
    "label": "Table / alternate Row Background",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.borderColor",
    "kind": "color",
    "tab": 5,
    "label": "Table / border Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.borderWidth",
    "kind": "number",
    "tab": 5,
    "label": "Table / border Width",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.horizontalPadding",
    "kind": "number",
    "tab": 5,
    "label": "Table / horizontal Padding",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.table.verticalPadding",
    "kind": "number",
    "tab": 5,
    "label": "Table / vertical Padding",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.math.color",
    "kind": "color",
    "tab": 6,
    "label": "Math / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.math.scale",
    "kind": "number",
    "tab": 6,
    "label": "Math / scale",
    "minimum": 0.01,
    "maximum": 1000
  },
  {
    "key": "style.headings.h1.font",
    "kind": "font",
    "tab": 2,
    "label": "Headings / h1 / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h1.color",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h1 / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h1.separatorVisible",
    "kind": "bool",
    "tab": 2,
    "label": "Headings / h1 / separator Visible",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h1.separatorColor",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h1 / separator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h1.separatorThickness",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h1 / separator Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h1.separatorSpacing",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h1 / separator Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h2.font",
    "kind": "font",
    "tab": 2,
    "label": "Headings / h2 / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h2.color",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h2 / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h2.separatorVisible",
    "kind": "bool",
    "tab": 2,
    "label": "Headings / h2 / separator Visible",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h2.separatorColor",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h2 / separator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h2.separatorThickness",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h2 / separator Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h2.separatorSpacing",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h2 / separator Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h3.font",
    "kind": "font",
    "tab": 2,
    "label": "Headings / h3 / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h3.color",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h3 / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h3.separatorVisible",
    "kind": "bool",
    "tab": 2,
    "label": "Headings / h3 / separator Visible",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h3.separatorColor",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h3 / separator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h3.separatorThickness",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h3 / separator Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h3.separatorSpacing",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h3 / separator Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h4.font",
    "kind": "font",
    "tab": 2,
    "label": "Headings / h4 / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h4.color",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h4 / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h4.separatorVisible",
    "kind": "bool",
    "tab": 2,
    "label": "Headings / h4 / separator Visible",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h4.separatorColor",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h4 / separator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h4.separatorThickness",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h4 / separator Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h4.separatorSpacing",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h4 / separator Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h5.font",
    "kind": "font",
    "tab": 2,
    "label": "Headings / h5 / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h5.color",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h5 / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h5.separatorVisible",
    "kind": "bool",
    "tab": 2,
    "label": "Headings / h5 / separator Visible",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h5.separatorColor",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h5 / separator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h5.separatorThickness",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h5 / separator Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h5.separatorSpacing",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h5 / separator Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h6.font",
    "kind": "font",
    "tab": 2,
    "label": "Headings / h6 / font",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h6.color",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h6 / color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h6.separatorVisible",
    "kind": "bool",
    "tab": 2,
    "label": "Headings / h6 / separator Visible",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h6.separatorColor",
    "kind": "color",
    "tab": 2,
    "label": "Headings / h6 / separator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h6.separatorThickness",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h6 / separator Thickness",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "style.headings.h6.separatorSpacing",
    "kind": "number",
    "tab": 2,
    "label": "Headings / h6 / separator Spacing",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.keywordColor",
    "kind": "color",
    "tab": 4,
    "label": "Keyword Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.typeColor",
    "kind": "color",
    "tab": 4,
    "label": "Type Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.functionColor",
    "kind": "color",
    "tab": 4,
    "label": "Function Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.variableColor",
    "kind": "color",
    "tab": 4,
    "label": "Variable Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.builtInColor",
    "kind": "color",
    "tab": 4,
    "label": "Built In Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.stringColor",
    "kind": "color",
    "tab": 4,
    "label": "String Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.numberColor",
    "kind": "color",
    "tab": 4,
    "label": "Number Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.commentColor",
    "kind": "color",
    "tab": 4,
    "label": "Comment Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.preprocessorColor",
    "kind": "color",
    "tab": 4,
    "label": "Preprocessor Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.operatorColor",
    "kind": "color",
    "tab": 4,
    "label": "Operator Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.constantColor",
    "kind": "color",
    "tab": 4,
    "label": "Constant Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "syntax.errorColor",
    "kind": "color",
    "tab": 4,
    "label": "Error Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "view.searchHighlightColor",
    "kind": "color",
    "tab": 1,
    "label": "Search Highlight Color",
    "minimum": 0,
    "maximum": 1000
  },
  {
    "key": "view.currentSearchHighlightColor",
    "kind": "color",
    "tab": 1,
    "label": "Current Search Highlight Color",
    "minimum": 0,
    "maximum": 1000
  }
])json";
