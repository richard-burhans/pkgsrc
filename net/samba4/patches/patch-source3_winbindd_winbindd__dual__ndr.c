$NetBSD$

Fix winbindd on SunOS, where getpeername() on a socketpair() returns a
zero length address.  tsocket_address_bsd_from_sockaddr() rejects it,
so every request to a winbindd domain child fails and wbinfo -t/-P and
NSS lookups break.  Treat it as an unnamed AF_UNIX address.

Also map errno rather than the -1 return value, so failures here are
not all logged as NT_STATUS_ACCESS_DENIED.

--- source3/winbindd/winbindd_dual_ndr.c.orig
+++ source3/winbindd/winbindd_dual_ndr.c
@@ -477,17 +477,28 @@ static NTSTATUS set_remote_addresses(str
 	ssa = (struct samba_sockaddr) { .sa_socklen = sizeof(ssa.u.ss), };
 	ret = getpeername(sock, &ssa.u.sa, &ssa.sa_socklen);
 	if (ret != 0) {
-		status = map_nt_error_from_unix(ret);
+		status = map_nt_error_from_unix(errno);
 		DBG_ERR("getpeername failed: %s\n", nt_errstr(status));
 		return status;
 	}
+	if (ssa.sa_socklen == 0) {
+		/*
+		 * sock is one end of a socketpair(). On illumos and
+		 * Solaris getpeername() reports the unnamed peer with
+		 * a zero length address, which
+		 * tsocket_address_bsd_from_sockaddr() rejects. Treat
+		 * it as the unnamed AF_UNIX address Linux returns.
+		 */
+		ssa.u.sa.sa_family = AF_UNIX;
+		ssa.sa_socklen = sizeof(ssa.u.sa.sa_family);
+	}
 
 	ret = tsocket_address_bsd_from_sockaddr(conn,
 						&ssa.u.sa,
 						ssa.sa_socklen,
 						&remote);
 	if (ret != 0) {
-		status = map_nt_error_from_unix(ret);
+		status = map_nt_error_from_unix(errno);
 		DBG_ERR("tsocket_address_bsd_from_sockaddr failed: %s\n",
 			nt_errstr(status));
 		return status;
@@ -496,7 +507,7 @@ static NTSTATUS set_remote_addresses(str
 	ssa = (struct samba_sockaddr) { .sa_socklen = sizeof(ssa.u.ss), };
 	ret = getsockname(sock, &ssa.u.sa, &ssa.sa_socklen);
 	if (ret != 0) {
-		status = map_nt_error_from_unix(ret);
+		status = map_nt_error_from_unix(errno);
 		DBG_ERR("getsockname failed: %s\n", nt_errstr(status));
 		return status;
 	}
@@ -506,7 +517,7 @@ static NTSTATUS set_remote_addresses(str
 						ssa.sa_socklen,
 						&local);
 	if (ret != 0) {
-		status = map_nt_error_from_unix(ret);
+		status = map_nt_error_from_unix(errno);
 		DBG_ERR("tsocket_address_bsd_from_sockaddr failed: %s\n",
 			nt_errstr(status));
 		return status;
