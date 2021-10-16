Function RandomPass

	dim i

	p=""

	for i=0 to 14
	
		c=int(rnd()*95)

		p=p & chr(32+c)
		
	next

	RandomPass=p

End Function

Set objRootDSE = GetObject("LDAP://RootDSE") 
  
nc=objRootDSE.Get("defaultNamingContext")
  
Set objContainer = GetObject("LDAP://cn=Users," & nc)

On Error Resume Next

For i = 1 To 500
    
    
    Set objLeaf = objContainer.Create("User", "cn=UserNo" & i) 
    objLeaf.Put "sAMAccountName", "UserNo" & i 
    objLeaf.SetInfo 
  
	For j = 1 to 5
		p=RandomPass()
		objLeaf.SetPassword p
		objLeaf.pwdLastSet=0 
		objLeaf.SetInfo 
	Next

    if (i mod 100) = 0 then
	WScript.Echo i & " passwords set."
    end if

Next 
