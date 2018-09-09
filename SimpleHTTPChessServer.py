"""HTTP Chess Server - based on CGIHTTPServer.py

This module builds on SimpleHTTPServer by implementing GET and POST
requests to cgi-bin scripts.

In all cases, the implementation is intentionally naive -- all
requests are executed sychronously.

SECURITY WARNING: DON'T USE THIS CODE UNLESS YOU ARE INSIDE A FIREWALL
-- it may execute arbitrary Python code or external programs.

"""


__version__ = "0.4"

__all__ = ["CGIHTTPRequestHandler"]

import os
import sys
import urllib
import BaseHTTPServer
import SimpleHTTPServer
import select

import cgiskak, cgireview


class CGIHTTPRequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    """Complete HTTP server with GET, HEAD and POST commands.
    GET and HEAD also support running CGI scripts.
    The POST command is *only* implemented for CGI scripts.

    """

    # Make rfile unbuffered -- we need to read one line and then pass
    # the rest to a subprocess, so we can't use buffered input.
    rbufsize = 0

    def do_POST(self):
        """Serve a POST request.
        This is only implemented for CGI scripts.
        """

        if self.is_cgi():
            self.run_cgi()
        else:
            self.send_error(501, "Can only POST to CGI scripts")

    def send_head(self):
        """Version of send_head that support CGI scripts"""
        if self.is_cgi():
            return self.run_cgi()
        else:
            return SimpleHTTPServer.SimpleHTTPRequestHandler.send_head(self)

    def script_ok(self, path):
        for scr in ["/cgiskak.py", "/cgireview.py"]:
            if path[0:len(scr)]==scr:
                return True
        return False

    def is_cgi(self):
        """Test whether self.path corresponds to a CGI script.

        Return a tuple (dir, rest) if self.path requires running a
        CGI script, None if not.  Note that rest begins with a
        slash if it is not empty.

        The default implementation tests whether the path
        begins with one of the strings in the cgi_directories list
        (and the next character is a '/' or the end of the string).

        """
        if not self.script_ok(self.path):
            return False
            
        path = self.path

        for x in ['', '/cgi-bin', '/htbin']:   # cgi_directories
            i = len(x)
            if path[:i] == x and (not path[i:] or path[i] == '/'):
                self.cgi_info = path[:i], path[i+1:]
                return True
        return False

    def run_cgi(self):
        """Execute a CGI script."""
        dir, rest = self.cgi_info
        i = rest.rfind('?')
        if i >= 0:
            rest, query = rest[:i], rest[i+1:]
        else:
            query = ''
        i = rest.find('/')
        if i >= 0:
            script, rest = rest[:i], rest[i:]
        else:
            script, rest = rest, ''
        scriptname = dir + '/' + script
        scriptfile = self.translate_path(scriptname)

        # Reference: http://hoohoo.ncsa.uiuc.edu/cgi/env.html
        # XXX Much of the following could be prepared ahead of time!
        env = {}
        env['SERVER_SOFTWARE'] = self.version_string()
        env['SERVER_NAME'] = self.server.server_name
        env['GATEWAY_INTERFACE'] = 'CGI/1.1'
        env['SERVER_PROTOCOL'] = self.protocol_version
        env['SERVER_PORT'] = str(self.server.server_port)
        env['REQUEST_METHOD'] = self.command
        uqrest = urllib.unquote(rest)
        env['PATH_INFO'] = uqrest
        env['PATH_TRANSLATED'] = self.translate_path(uqrest)
        env['SCRIPT_NAME'] = scriptname
        if query:
            env['QUERY_STRING'] = query
        host = self.address_string()
        if host != self.client_address[0]:
            env['REMOTE_HOST'] = host
        env['REMOTE_ADDR'] = self.client_address[0]
        authorization = self.headers.getheader("authorization")
        if authorization:
            authorization = authorization.split()
            if len(authorization) == 2:
                import base64, binascii
                env['AUTH_TYPE'] = authorization[0]
                if authorization[0].lower() == "basic":
                    try:
                        authorization = base64.decodestring(authorization[1])
                    except binascii.Error:
                        pass
                    else:
                        authorization = authorization.split(':')
                        if len(authorization) == 2:
                            env['REMOTE_USER'] = authorization[0]
        # XXX REMOTE_IDENT
        if self.headers.typeheader is None:
            env['CONTENT_TYPE'] = self.headers.type
        else:
            env['CONTENT_TYPE'] = self.headers.typeheader
        length = self.headers.getheader('content-length')
        if length:
            env['CONTENT_LENGTH'] = length
        accept = []
        for line in self.headers.getallmatchingheaders('accept'):
            if line[:1] in "\t\n\r ":
                accept.append(line.strip())
            else:
                accept = accept + line[7:].split(',')
        env['HTTP_ACCEPT'] = ','.join(accept)
        ua = self.headers.getheader('user-agent')
        if ua:
            env['HTTP_USER_AGENT'] = ua
        co = filter(None, self.headers.getheaders('cookie'))
        if co:
            env['HTTP_COOKIE'] = ', '.join(co)
        # XXX Other HTTP_* headers
        # Since we're setting the env in the parent, provide empty
        # values to override previously set values
        for k in ('QUERY_STRING', 'REMOTE_HOST', 'CONTENT_LENGTH',
                  'HTTP_USER_AGENT', 'HTTP_COOKIE'):
            env.setdefault(k, "")
        os.environ.update(env)

        self.send_response(200, "Script output follows")

        decoded_query = query.replace('+', ' ')

        # execute script in this process
        save_argv = sys.argv
        save_stdin = sys.stdin
        save_stdout = sys.stdout
        save_stderr = sys.stderr
        try:
            save_cwd = os.getcwd()
            try:
                sys.argv = [scriptfile]
                if '=' not in decoded_query:
                    sys.argv.append(decoded_query)
                sys.stdout = self.wfile
                sys.stdin = self.rfile
                #print "Content-Type: text/html\n"
                #print "<html><head></head><body>Hello World!</body></html>"
                #execfile(scriptfile, {"__name__": "__main__"})
                if script=="cgireview.py":
                    cgireview.main()
                else:
                    cgiskak.main()
            finally:
                sys.argv = save_argv
                sys.stdin = save_stdin
                sys.stdout = save_stdout
                sys.stderr = save_stderr
                os.chdir(save_cwd)
        except SystemExit, sts:
            self.log_error("CGI script exit status %s", str(sts))
        else:
            self.log_message("CGI script exited OK")


def test(HandlerClass = CGIHTTPRequestHandler,
         ServerClass = BaseHTTPServer.HTTPServer):
    SimpleHTTPServer.test(HandlerClass, ServerClass)

if __name__ == '__main__':
    test()
