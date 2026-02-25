<!--
    //---------------------------------------------------------------------
    // Logon Script Stuff
    //---------------------------------------------------------------------
var gMaxUsers = 0;
var currentLogonUser = null;
var KC_DOWN     = 40;
var KC_UP       = 38;
var KC_RETURN   = 13;
var KC_SPACE    = 32;
var KC_ESCAPE   = 27;
var KC_SHIFT    = 16;
var KC_CONTROL  = 17;
var passwordTimeout = 0;
var kPasswordTimeoutLength = 30000;
var reminderTimeout = 0;
var kReminderTimeoutLength = 10000;
var gNumLargeIcons = 5;
var gNumSmallIcons = 5;
var gTopIcon = 130;
var gLeftIcon = 200;
var kLargeIconHeight = 75;
var kSmallIconHeight = 45;
var gPasswordAreaLeft = 130;

var kRestartComputerMessage = "Restart the computer";
var kTurnOffComputerMessage = "Turn off the computer";
var kShutDownComputerMessage = "Complelely shut down the computer";
//
// Calculate the sizes and positions we will use for laying out
// the names on the screen.  Eventually, this should be done by the 
// final layout engine
//
function calculateSizes()
{
    // calculate the max number of large icons before we need to switch to small icons
    gNumLargeIcons = (screen.availHeight - gTopIcon + 100) / kLargeIconHeight;

    // calculate the max number of small icons before we start truncating
    gNumSmallIcons = (screen.availHeight - gTopIcon + 100) / kSmallIconHeight;

    // calculate horizontal position for icons and the password area
    gLeftIcon = Math.max(screen.availWidth / 4, 200);
    gPasswordAreaLeft = 260; //gLeftIcon + 260;
}


// if the user space when an element has focus, act like we clicked it.
function clickSelf(el)
{
    if((window.event.keyCode == KC_RETURN || window.event.keyCode == KC_SPACE)
        && event.srcElement == el)
    {
        el.click();
    }
}


// User clicked the link to show their password reminder
function clickForgot()
{
    if (currentLogonUser)
    {
        idPwd.focus();
        idBalloon.style.display = "";
		idBalloon.Popup("idForgotPwd", currentLogonUser.reminder);
    }
}

// User clicked the element that we use to quit the hta
function clickClose()
{
    if (window.event.shiftKey)
    {
        window.parent.close();
    }
}

// User clicked the 'turn off the machine' link
function turnOff()
{
    var objLocalMachine = new ActiveXObject("Shell.LocalMachine");

    resetFilters();

    if (objLocalMachine)
    {
        try 
        {
            if (window.event.shiftKey && !window.event.ctrlKey)
            {
                objLocalMachine.Restart();
            }
            else if (window.event.ctrlKey && !window.event.shiftKey)
            {
                objLocalMachine.Shutdown();
            }
            else
            {
                objLocalMachine.Hibernate();
            }
        }
        catch (objException)
        {
        }
    }
}


// Mouse rolls over a user 'button'.  Change the style to highlight the 
// user and make it looks like it would be a nifty place to click
function userMouseOver(el)
{
    if (el != currentLogonUser && currentLogonUser != null)
    {
        var elId = el.id.substr(4);
        document.all["picture"+elId].style.filter = "alpha(Opacity=66)";
    }
}

// Mouse rolls out of the user's 'button'.  Change the style to unhighlight
// the user and make it look less nifty
function userMouseOut(el)
{
    if (el != currentLogonUser && currentLogonUser != null)
    {
        var elId = el.id.substr(4);
        document.all["picture"+elId].style.filter = "alpha(Opacity=25)";
    }
    
}

// the user needs to enter a password to log in.  Show the password fields 
// and the reminder link if there is a reminder to show them.
function showPasswordFields(el)
{
    if (passwordTimeout != 0)
    {
        window.clearTimeout(passwordTimeout);
        passwordTimeout = 0;
    }
    if (reminderTimeout != 0)
    {
        window.clearTimeout(reminderTimeout);
        reminderTimeout = 0;
    }
    idPasswordArea.style.left = gPasswordAreaLeft;
    idPasswordArea.style.top = el.offsetTop + 4;
    idPasswordArea.style.display = "";
    idPwd.focus();
    idPwd.tabIndex = el.tabIndex + 1;
    goButton.tabIndex = el.tabIndex + 2;
    idForgotPwd.tabIndex = el.tabIndex + 4;
    idPwd.value = "";
        
    if (currentLogonUser.reminder == "")
    {
        idForgotPwd.style.display = "none";
    }
    else
    {
        idForgotPwd.style.display = "";
        reminderTimeout = window.setTimeout(clickForgot, kReminderTimeoutLength);
    }
    passwordTimeout = window.setTimeout(resetFilters, kPasswordTimeoutLength);
}

// Remove the dimming from the unselected buttons
function resetFilters()
{
    for (i = 0; i < gMaxUsers; i++)
    {
        document.all["picture"+i].style.filter = "none";
    }
    idPasswordArea.style.display="none";
    idPwd.value ="";
    currentLogonUser = null;
    idBalloon.style.display = "none";
}

// the user was clicked and we have a password.  Lets try logging them in.
// if the logon does not work, show the password field, clear it, and let 
// them try again.
function logonUser()
{
    var objShellOm = new ActiveXObject("Shell.Users");
    
    for (i = 0; i < objShellOm.length; i++)
    {
        if (objShellOm(i).setting("LoginName") == currentLogonUser.logon)
        {
            var vLoginResult = objShellOm(i).Login(idPwd.value);
            
            if (vLoginResult == 0)
            {
                if (idPwd.value != "")
                {
                    idBalloon.style.display = "";
                    idBalloon.Popup("idPwd", idPasswordErrorString.innerHTML);
                }
                showPasswordFields(currentLogonUser);
            }
            else
            {
                window.parent.close();
            }
        }
    }   
}

// The user wants to logon as guest.  Find the user object named
// Guest, then try to logon as them
function logonGuest()
{
    var objShellOm = new ActiveXObject("Shell.Users");
    
    resetFilters();
    for (i = 0; i < objShellOm.length; i++)
    {
        if (objShellOm(i).setting("LoginName") == "Guest")
        {
            var vLoginResult = objShellOm(i).Login("");
            
            if (vLoginResult != 0)
            {
                window.parent.close();
            }
        }
    }   
}

// If they hit the enter key in the password edit field, try the logon
function pwdKeyPress()
{
    if(window.event.keyCode == KC_RETURN)
    {
        logonUser();
    }
    else
    {
        if (window.event.keyCode == KC_ESCAPE)
        {
            window.clearTimeout(passwordTimeout);
            passwordTimeout = 0;
            if (reminderTimeout != 0)
            {
                window.clearTimeout(reminderTimeout);
                reminderTimeout = 0;
            }
            resetFilters();
        }
        
        //reset the password field timeout so we will go back to the full list if
        // the user hasn't typed anything for 30 seconds or so
        if (passwordTimeout != 0)
        {
            window.clearTimeout(passwordTimeout);
            passwordTimeout = window.setTimeout(resetFilters, kPasswordTimeoutLength);
        }
        if (reminderTimeout != 0)
        {
            window.clearTimeout(reminderTimeout);
            reminderTimeout = window.setTimeout(clickForgot, kReminderTimeoutLength);
        }
        
        if (idBalloon.style.display != "none")
        {
            idBalloon.style.display = "none"; 
        }
    }
}


// check for releasing the shift key to reset the Turn Off message
function bodyKeyUp()
{
    var keyCode = window.event.keyCode;

    if (keyCode == KC_SHIFT)
    {
        if (window.event.ctrlKey && !PwdHasFocus())
        {
            idTurnOffMessage.innerText = kShutDownComputerMessage;    
        }
        else
        {
            idTurnOffMessage.innerText = kTurnOffComputerMessage;    
        }
    }
    
    if (keyCode == KC_CONTROL)
    {
        if (window.event.shiftKey && !PwdHasFocus())
        {
            idTurnOffMessage.innerText = kRestartComputerMessage;    
        }
        else
        {
            idTurnOffMessage.innerText = kTurnOffComputerMessage;    
        }
    }
}

function PwdHasFocus()
{
    if (document.activeElement == idPwd)
        return true;
    else
        return false;
}

// if the user clicks the up or down button, switch to the next
// current user.                    
function bodyKeyDown()
{
    var keyCode = window.event.keyCode;

    if (keyCode == KC_ESCAPE )
    {   
        resetFilters();
    }

    if (keyCode == KC_SHIFT && !PwdHasFocus())
    {
        if (window.event.ctrlKey)
        {
            idTurnOffMessage.innerText = kTurnOffComputerMessage;    
        }
        else
        {
            idTurnOffMessage.innerText = kRestartComputerMessage;    
        }
    }
    
    if (keyCode == KC_CONTROL && !PwdHasFocus())
    {
        if (window.event.shiftKey)
        {
            idTurnOffMessage.innerText = kTurnOffComputerMessage;    
        }
        else
        {
            idTurnOffMessage.innerText = kShutDownComputerMessage;    
        }
    }
/*    
    if (keyCode == KC_DOWN || keyCode == KC_UP)
    {
        if (currentLogonUser)
        {
            // get the currently selected user id
            var currId = currentLogonUser.id;
            
            // chop off the 'user' part of userX
            currId = currId.slice(4);
            
            // make it a number
            var idNum = currId.valueOf();
            
            // increment or decrement as appropriate
            if (keyCode == KC_UP)
            {
                idNum --;
                if (idNum < 0)
                {
                    idNum = gMaxUsers - 1;
                }
            }
            else
            {
                idNum ++;
                if (idNum >= gMaxUsers)
                {
                    idNum = 0;
                }
            }
            
            var selEl = document.all["user"+idNum];
            
            if (selEl)
            {
                resetFilters();
                clickUser(selEl, false);
            }
        }
    }   
*/
}


// If the user hit the enter key when one of the user 'buttons' are pressed,
// act like the user was actually clicked.
function userKeyPress()
{
    if(window.event.keyCode == KC_RETURN)
    {
        clickUser(window.event.srcElement, true);
    }
}



// the user was clicked.  Highlight them as clicked and 
// try logging in with a blank password
function clickUser(el, bAutoLogin)
{
    var elId = -1;
    for (i = 0; i < gMaxUsers; i++)
    {
        if (document.all["user"+i] != el)
        {
            document.all["picture"+i].style.filter = "alpha(Opacity=25)";
        }
        else
        {
            elId = i;
        }
    }
    
    if (elId != -1)
        document.all["picture"+elId].style.filter = "none"
    idPwd.value = "";
    currentLogonUser = el;
    el.style.filter = "none";
    idBalloon.style.display = "none";
    userMouseOver(el);
    
    if (bAutoLogin)
    {
        logonUser();
    }
}

// The requested picture could not be loaded.  Try the 
// gif version of the same file name.  If that doesn't
// work, we'll try a hard coded picture.
function onPictureLoadError(el)
{
    el.reload++;

    if (el.reload == 1)
    {
        el.src = el.src.replace(".jpg", ".gif");
    }
    else if (el.reload == 2)
    {
        el.src = "DefaultUser.gif";
    }
    else
    {
        el.onError = "";    //turn off the error handler.
    }
}


// Create a user button.  Build up the div that contains
// the user's picture, name and e-mail notification.
//
function FillInUserFrame(elNum, logon, name, password, imgUrl, noteImg, reminderTxt, imgSize)
{
    var element ;
    var elHtml = '<div id=user' +elNum+ ' class="Unframed" tabIndex= ' + (elNum+1) * 10 + '  style="WIDTH: 140px" onmouseover="userMouseOver(this)" onclick="clickUser(this, true)" onmouseout="userMouseOut(this)" onkeypress="userKeyPress()" language="Javascript1.2"> </div>'; 
    var imgWidth, imgHeight;

    if (imgSize)	
    {
        imgWidth = 56;
        imgHeight = 60;
    }
    else
    {
        imgWidth = 28;
        imgHeight = 30;
    }
    var userHtml = 
    '<div class="UserCell"> ' +
    '<table id=userEntry border="0" width="171%" cellspacing="0" cellpadding="2"> ' +
    '   <tr> ' + 
    '       <td width="56" rowspan="3"><p align="center">' +
    '           <img id=picture' +elNum+ ' reload=0 onError="onPictureLoadError(this)" border="0" src="'+ imgUrl + '" width=' + imgWidth +' height=' + imgHeight + '  style="margin-right:4px;">' +
    '       </td> ' +
    '       <td width="110%" valign="top"> ' +
    '           <font face="Trebuchet MS" color="777777" size="4">' + name + '</font> ' + 
    '       </td>' +
    '   </tr> ' +
    '   <tr>  ' + 
    '       <td width="84%"> ' + 
    '       </td> ' +
    '   </tr> ' +
    '   <tr> ' + 
    '       <td width="84%"> ';
    
    if (noteImg != "")
    {
        userHtml += '<img border="0" src="'+noteImg+'" width="16" height="11">';
    }
        
    userHtml += 
    '       </td> ' +
    '   </tr> ' +
    '</table></div>';

    element = document.createElement(elHtml);   
    if (element)
    {   
        element.password = password;
        element.username = name;
        element.logon = logon;
        element.reminder = reminderTxt;
        element.style.display = "";
        idElemInsert.insertAdjacentElement("BeforeBegin" , element);
        element.innerHTML = userHtml;
    }
}


function OnLoadFrame()
{
    var objShellOm = new ActiveXObject("Shell.Users");
    var cUsers = 0;
    var cLogins = 0;
    var bSeenGuest = 0;
    var guestPicture = "";

    calculateSizes();

    if (null == objShellOm)
    {
        alert("ERROR: Can't load user list");
    }
    else
    {
        cUsers = objShellOm.length;

        if (cUsers > gNumSmallIcons)
            cUsers = gNumSmallIcons;
            
        for (i = 0; i < cUsers; i ++)
        {
            var currUser = objShellOm(i);
            var loginName = currUser.setting("LoginName");
            var displayName = currUser.setting("DisplayName");
            var imgPath = currUser.setting("Picture");
            var pwdHint;
            try 
            {
                pwdHint = currUser.setting("Hint");
            }
            catch (objException)
            {
                pwdHint = "";
            }
                    
            if (loginName == "Guest")
            {
                bSeenGuest = 1;
                guestPicture = imgPath;
            }
                
            if (loginName != "Guest")
            {
                if (displayName.length == 0)
                {
                    displayName = currUser.setting("LoginName"); 
                }
            
                FillInUserFrame(cLogins++, loginName, displayName, "*",imgPath , "", pwdHint, 1);
            }
        }

        // if there is only one user, automatically select them and try to log
        // in as them.
        // BUGBUG - this will be not so good for autologoff in the single user case
        // with no password since there will be no way back to the logon screen 
        // to shut down.
        if (cLogins == 1)
        {
            user0.focus();
            if (idAppLogon.commandLine.indexOf("FirstLaunch") >= 0)
                clickUser(user0, true);  //commented out pending a fix to shutdown from the shell
            else
                clickUser(user0, false);
        }

        if (bSeenGuest)
        {
            FillInUserFrame(cLogins++, "Guest", "Guest", "*", guestPicture, "", "", 1);        
        }

    }
    
    idBalloon.style.display = "none";

    gMaxUsers = cLogins;

}

function setMachineName()
{
    var objLocalMachine = new ActiveXObject("Shell.LocalMachine");

    idMachineName.innerText = objLocalMachine.MachineName;
}

// OnLoad handler for the page.  Create the buttons for each user on the system
// Ignore Guest (because there is a link for that) and Administrator (because
// you should not log in as Admin
function OnLoad()
{
    var objLocalMachine = new ActiveXObject("Shell.LocalMachine");
    if (objLocalMachine)
    {
        try 
        {
            objLocalMachine.SignalHibernateCommence();
        }
        catch (objException)
        {
        }
    }
    
    idTurnoff.style.top = screen.availHeight-100;
    idElemInsert.className = "UserCell";
    idBalloon.style.display="none";
    setMachineName();
    OnLoadFrame();
    idTurnoff.tabIndex = 15000;
}


function rollIn(el)
{
    var ms = navigator.appVersion.indexOf("MSIE")
    ie4 = (ms>0) && (parseInt(navigator.appVersion.substring(ms+5, ms+6)) >= 4)
    if(ie4)
    {
        el.initstyle=el.style.cssText;el.style.cssText=el.fprolloverstyle
    }
}
function rollOut(el)
{
    var ms = navigator.appVersion.indexOf("MSIE")
    ie4 = (ms>0) && (parseInt(navigator.appVersion.substring(ms+5, ms+6)) >= 4)
    if(ie4)
    {
        el.style.cssText=el.initstyle
    }
}
//-->


