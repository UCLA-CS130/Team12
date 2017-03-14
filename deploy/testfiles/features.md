# CS130 winter 2017

## TEAM 12 Assignment 9

### Feature 1 Markdown Rendering
 * Used [cpp_markdown](https://github.com/sevenjay/cpp-markdown)
 * This page is written in markdown.
   * Details: 
     1. A request for a .md file triggers the cpp_markdown module
     2. The module generates a c++ stream and is converted to a string
     3. The string is the rendered html page and written back as the response 

### Feature 2 Python Interpretation (main feature)
 * [Here](http://localhost:8012/python)
 * Type a Python script in the textaread and press the **enter** button beside the text box.
 * The input in the text area is send to the server and executed by python interpreter.
 * The result from standard output is redirected and sent back to the front end.
   * Details:
     1. A dedicated handler is implemented to handle python request
     2. The python page is wrtten in html+javascript. <br /> 
	The page is specified by the config and read into the handler on start up
     3. When a GET request is received by python handler, the python page is written back.
     4. When a POST request is sent to the server by javascript in the page, the python interpreter is triggered. The standard output is redirected and saved into a string and then written back to the client.
