
//global array: legal=[(frm1,to1),(frm2,to2)]
var legal=[];
var board=[];

var from=-1;
var sq_from="";
var sq_from_src="";

var req;

function loadXMLDoc(url) 
{
    // branch for native XMLHttpRequest object
    if (window.XMLHttpRequest) {
        req = new XMLHttpRequest();
        req.onreadystatechange = processReqChange;
        req.open("GET", url, true);
        req.send(null);
    // branch for IE/Windows ActiveX version
    } else if (window.ActiveXObject) {
        req = new ActiveXObject("Microsoft.XMLHTTP");
        if (req) {
            req.onreadystatechange = processReqChange;
            req.open("GET", url, true);
            req.send();
        }
    } else {
        alert("your browser does not support XMLHttpRequest");
    }
}

function draw_board(newboard, frm, to, status)
{
    board=newboard;
    for (i=0; i<board.length; i++) 
        draw_piece("", i, board.charAt(i));
    draw_piece("pressed-", to, board.charAt(to));
    draw_piece("pressed-", frm, board.charAt(frm));
    set_status(status);
}

function processReqChange() 
{
    // only if req shows "complete"
    if (req.readyState == 4) {
        // only if "OK"
        if (req.status == 200) {
            response  = req.responseXML.documentElement;
            board=response.getElementsByTagName('board')[0].firstChild.data;
            status=response.getElementsByTagName('status')[0].firstChild.data;
            sx=response.getElementsByTagName('lastFrom')[0].firstChild.data;
            sy=response.getElementsByTagName('lastTo')[0].firstChild.data;
            lx=parseInt(sx,10);
            ly=parseInt(sy,10);
            s=response.getElementsByTagName('legal')[0].firstChild.data;
            eval("legal="+s);
            for (i=0; i<board.length; i++)
                draw_piece("", i, board.charAt(i));
            if (lx>=0)
                draw_piece("pressed-", lx, board.charAt(lx));
            if (ly>=0) 
                draw_piece("pressed-", ly, board.charAt(ly));
            set_status(status);
        } else {
            alert("There was a problem retrieving the XML data:\n" + req.statusText);
        }
    }
}


function get_from_idx(n)
{
    for (i=0; i<legal.length; i++)
    {
        m=legal[i];
        if (m[0]==n)
            return i;
    }
    return -1;
}

function get_move_idx(from,to)
{
    for (i=0; i<legal.length; i++)
    {
        m=legal[i];
        if (m[0]==from && m[1]==to)
            return i;
    }
    return -1;
}

function set_pressed(n)
{
    name="sq"+n.toString(10)
    if (sq_from!="") 
        document.images[sq_from].src=sq_from_src;
    sq_from_src=document.images[name].src
    sq_from=name;
    draw_piece("pressed-", n, board.charAt(n));
}

function draw_piece(is_pressed, n, type)
{
    root="/static/bitmaps/";
    name="sq"+n.toString(10);
    x=7-Math.floor(n/8);
    y=n % 8; 
    if (type!='.') {
        if (type==type.toUpperCase())
            c='w';
        else
            c='b';
    } else 
        c='';
    if (x % 2 == y % 2)
        document.images[name].src=root+is_pressed+"b-"+c+type+".gif";
    else
        document.images[name].src=root+is_pressed+"w-"+c+type+".gif";
}

function set_status(txt)
{
    var node=document.getElementById('status')
    node.childNodes[0].data=txt;
}

function change_side()
{
    set_status('wait, processing...');
    from=-1;
    sq_from="";
    //loadXMLDoc("cgiskak.py?xml=1&change=1;")
    loadXMLDoc("change")
    legal=[]
}


function new_game()
{
    set_status('wait, processing...');
    from=-1;
    sq_from="";
    //loadXMLDoc("cgiskak.py?xml=1&new=1;")
    loadXMLDoc("new")
    legal=[]
}

function set_cursor(n) 
{
    name="sq"+n.toString(10)
    if (get_from_idx(n)!=-1) {
       if (window.ActiveXObject) 
           document.images[name].style.cursor="hand";
       else
           document.images[name].style.cursor="pointer";
    } else
       document.images[name].style.cursor="default";
}

function move(n)
{
    if (from==-1)
    {
        i=get_from_idx(n)
        if (i>=0)
        {
            from=n;
            set_pressed(from);
        }
    }
    else
    {
        if (from==n) {
            draw_piece("", n, board.charAt(from));
            from=-1;
            return ;
        }
        i=get_move_idx(from,n);
        if (i>=0)
        {
            draw_piece("pressed-", n, board.charAt(from));
            draw_piece("pressed-", from, ".");
      
            set_status('wait, processing...');
            from=-1;
            sq_from="";
            //loadXMLDoc("cgiskak.py?xml=1&move="+legal[i]) ;
            //loadXMLDoc("move/"+legal[i]) ;
            loadXMLDoc("move/"+legal[i][0]+'/'+legal[i][1]) ;
            legal=[]
            //document.choosemove.move.selectedIndex=i;
            //document.choosemo.submit();
        }
        else
        {
            i=get_from_idx(n);
            if (i>=0)
            {
                from=n;
                //document.choosemove.move.selectedIndex=i;
                set_pressed(from);
            }
            else 
                from=-1;
        }
    }
}
