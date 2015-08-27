<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"  xmlns:n="http://schemas.microsoft.com/wix/2003/01/wi" exclude-result-prefixes="n">
	<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>
  <xsl:template match="/">
     <xsl:apply-templates />
  </xsl:template>
  <xsl:template match="n:Wix">
    <Wix  xmlns="http://schemas.microsoft.com/wix/2003/01/wi">
       <xsl:apply-templates/>
     </Wix>
  </xsl:template>
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
