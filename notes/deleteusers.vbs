Set objRootDSE = GetObject("LDAP://RootDSE") 
  
nc=objRootDSE.Get("defaultNamingContext")
  
Set objContainer = GetObject("LDAP://cn=Users," & nc)

On Error Resume Next    

For i = 1 To 500
 
    objContainer.Delete "User", "cn=UserNo" & i
 
    if (i mod 1000) = 0 then
		WScript.Echo i & " users deleted."
    end if

Next 

