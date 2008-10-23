<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" encoding="ISO-8859-1" indent="yes"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>

  <xsl:variable name="href_base" select="''"/>
  <xsl:variable name="menu_name">Main Menu</xsl:variable>
<!--
 - returns the filename associated to an ID in the original file
 -->
  <xsl:template name="filename">
    <xsl:param name="name" select="string(@href)"/>
    <xsl:choose>
      <xsl:when test="$name = '#Introduction'">
        <xsl:text>index.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#releases'">
        <xsl:text>releases.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#downloads'">
        <xsl:text>downloads.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#architecture'">
        <xsl:text>architecture.html</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
<!--
 - The global title
 -->
  <xsl:variable name="globaltitle" select="string(/html/body/h1[1])"/>

<!--
  the main menu box
 -->
  <xsl:template name="menu">
      <ul class="l0">
        <li><div><a href="{$href_base}index.html" class="inactive">Home</a></div></li>
	<li> <a href="http://libvirt.org/" class="inactive">libvirt</a></li>
    <xsl:for-each select="/html/body/h2">
    <xsl:variable name="filename">
      <xsl:call-template name="filename">
	<xsl:with-param name="name" select="concat('#', string(a[1]/@name))"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="$filename != ''">
      <li>
        <div>
	<xsl:element name="a">
	  <xsl:attribute name="href">
	    <xsl:value-of select="$filename"/>
	  </xsl:attribute>
	  <xsl:attribute name="class">inactive</xsl:attribute>
	  <xsl:value-of select="."/>
	</xsl:element>
        </div>
      </li>
    </xsl:if>
    </xsl:for-each>
        <li> <a href="https://www.redhat.com/mailman/listinfo/libvir-list" class="inactive">Mailing list</a></li>
      </ul>
  </xsl:template>

<!--
  the page title
 -->

  <xsl:template name="titlebox">
    <xsl:param name="title"/>
    <h1 class="style1"><xsl:value-of select="$title"/></h1>
  </xsl:template>

<!--
 - Write the styles in the head
 -->
  <xsl:template name="style">
    <link rel="stylesheet" type="text/css" href="http://libvirt.org/qpid/main.css" />
    <link rel="SHORTCUT ICON" href="/32favicon.png" />
  </xsl:template>

<!--
 - The top section 
 -->
  <xsl:template name="top">
    <div id="top">
      <img src="{$href_base}libvirtHeader.png" alt="Libvirt the virtualization API" />
    </div>
  </xsl:template>

<!--
 - The top section for the main page
 -->
  <xsl:template name="topmain">
    <div id="topmain">
      <img src="{$href_base}libvirtLogo.png" alt="Libvirt the virtualization API" />
    </div>
  </xsl:template>

<!--
 - The bottom section
 -->
  <xsl:template name="bottom">
    <div id="bottom">
      <p class="p1"></p>
    </div> 
  </xsl:template>

<!--
 - Handling of nodes in the body after an H2
 - Open a new file and dump all the siblings up to the next H2
 -->
  <xsl:template name="subfile">
    <xsl:param name="header" select="following-sibling::h2[1]"/>
    <xsl:variable name="filename">
      <xsl:call-template name="filename">
        <xsl:with-param name="name" select="concat('#', string($header/a[1]/@name))"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="title">
      <xsl:value-of select="$header"/>
    </xsl:variable>
    <xsl:document href="{$filename}" method="xml" encoding="ISO-8859-1"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
      <html>
        <head>
          <xsl:call-template name="style"/>
          <xsl:element name="title">
            <xsl:value-of select="$title"/>
          </xsl:element>
          <meta name="description" content="libvirt, virtualization, virtualization API, qpid"/>
        </head>
	<body>
          <div id="header">
            <div id="headerLogo"/>
            <div id="headerSearch">
              <form action="{$href_base}search.php" enctype="application/x-www-form-urlencoded" method="get">
                <div>
                  <input id="query" name="query" type="text" size="12" value=""/>
                  <input id="submit" name="submit" type="submit" value="Search"/>
                </div>
              </form>
            </div>
          </div>
	  <div id="body">
            <div id="menu">
              <xsl:call-template name="menu"/>
            </div>
            <div id="content">
	      <xsl:call-template name="titlebox">
		<xsl:with-param name="title" select="$title"/>
	      </xsl:call-template>
	      <xsl:apply-templates mode="subfile" select="$header/following-sibling::*[preceding-sibling::h2[1] = $header and name() != 'h2' ]"/>
	    </div>
	  </div>
	  <div id="footer">
          </div>
	</body>
      </html>
    </xsl:document>
  </xsl:template>

  <xsl:template mode="subcontent" match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates mode="subcontent" select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template mode="content" match="@*|node()">
    <xsl:if test="name() != 'h1' and name() != 'h2'">
      <xsl:copy>
        <xsl:apply-templates mode="subcontent" select="@*|node()"/>
      </xsl:copy>
    </xsl:if>
  </xsl:template>

  <xsl:template mode="subfile" match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates mode="content" select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

<!--
 - Handling of the initial body and head HTML document
 -->
  <xsl:template match="body">
    <xsl:variable name="firsth2" select="./h2[1]"/>
    <body>
      <div id="header">
        <div id="headerLogo"/>
        <div id="headerSearch">
          <form action="{$href_base}search.php" enctype="application/x-www-form-urlencoded" method="get">
            <div>
              <input id="query" name="query" type="text" size="12" value=""/>
              <input id="submit" name="submit" type="submit" value="Search"/>
            </div>
          </form>
        </div>
      </div>
      <div id="body">
        <div id="menu">
          <xsl:call-template name="menu"/>
        </div>
        <div id="content">
          <xsl:apply-templates mode="content" select="($firsth2/preceding-sibling::*)"/>
          <xsl:for-each select="./h2">
            <xsl:call-template name="subfile">
	      <xsl:with-param name="header" select="."/>
            </xsl:call-template>
          </xsl:for-each>
	</div>
      </div>
      <div id="footer">
        <p id="sponsor">
          Sponsored by:<br/>
          Red Hat
        </p>
      </div>
    </body>
  </xsl:template>

  <xsl:template match="head">
  </xsl:template>
  <xsl:template match="html">
    <xsl:message>Generating the Web pages</xsl:message>
    <html>
      <head>
        <xsl:call-template name="style"/>
        <title>the virtualization qpid API</title>
        <meta name="description" content="libvirt, virtualization, virtualization API, qpid"/>
      </head>
      <xsl:apply-templates/>
    </html>
  </xsl:template>
</xsl:stylesheet>
