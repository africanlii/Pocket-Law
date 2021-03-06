<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns="http://www.w3.org/1999/xhtml"
 xmlns:xhtml="http://www.w3.org/1999/xhtml"
>
<!--
   This template will take and return HTML source, 
   sans any script tags markup. 
		$Id$
-->


<!-- Identity transformation template - let everything else pass through  -->
   <xsl:template match=" * | comment() | processing-instruction() | text()">
      <xsl:copy>
         <xsl:copy-of select="@*" />
         <xsl:apply-templates
         select=" * | comment() | processing-instruction() | text()" />
      </xsl:copy>
   </xsl:template>

   <xsl:template match="xhtml:script|xhtml:noscript"><!--  ignore --></xsl:template>

</xsl:stylesheet>

