<?xml version="1.0"?>

<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns="http://www.w3.org/TR/REC-html40"
  version="1.0"><xsl:output method="html"/>

<xsl:template match="/">
<html>
  <head>
    <title></title>
  </head>
  <body>
  <xsl:for-each select="CompatReport/HardBlocks">
             <h2><font color="black"><xsl:value-of select="Message"/></font></h2>
  </xsl:for-each>
  <xsl:for-each select="CompatReport/HardBlocks">
             <h3><font color="blue"><xsl:value-of select="AppHead"/></font></h3>
  </xsl:for-each>

  <xsl:for-each select="CompatReport/HardBlocks/AppNames/AppName">
              <font color="green"> <li><xsl:value-of select="Name"/></li></font>
  </xsl:for-each>

  <xsl:for-each select="CompatReport/HardBlocks/AppNames">
      <p><a href="#{generate-id(.)}">
	<font color="blue"><b><xsl:value-of select="./Help"/></b></font>
         </a>
      </p>
  </xsl:for-each>
  <br/><br/>



  <xsl:for-each select="CompatReport/SoftBlocks">
             <h2><font color="black"><xsl:value-of select="Message"/></font></h2>
  </xsl:for-each>
  <xsl:for-each select="CompatReport/SoftBlocks">
             <h3><font color="blue"><xsl:value-of select="AppHead"/></font></h3>
  </xsl:for-each>

  <xsl:for-each select="CompatReport/SoftBlocks/AppNames/AppName">
              <font color="green"> <li><xsl:value-of select="Name"/></li></font>
    </xsl:for-each>

    <xsl:for-each select="CompatReport/SoftBlocks/AppNames">
      <p><a href="#{generate-id(.)}">
        <font color="blue"><b><xsl:value-of select="./Help"/></b></font>
         </a>
      </p>
  </xsl:for-each>
  <br/><br/>


  <h2></h2>
  <xsl:for-each select="CompatReport/HardBlocks/AppNames">
    <h3><a name="{generate-id(.)}"><xsl:value-of select="CompatReport/HardBlocks/AppNames"/></a></h3>
         <p><xsl:value-of select="./About"/></p>
         <a href="#toc"><small>Back to issue list</small></a>
         <br/><br/><br/>
  </xsl:for-each>

  <h2></h2>
  <xsl:for-each select="CompatReport/SoftBlocks/AppNames">
    <h3><a name="{generate-id(.)}"><xsl:value-of select="CompatReport/SoftBlocks/AppNames"/></a></h3>
        
           
	 <p><xsl:value-of select="./About"/></p>
         <a href="#toc"><small>Back to issue list</small></a>
         <br/><br/><br/>
  </xsl:for-each>



  </body>
</html>
</xsl:template>

</xsl:stylesheet>
