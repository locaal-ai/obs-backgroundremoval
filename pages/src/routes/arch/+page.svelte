<script>
	import { base } from '$app/paths';
</script>

<svelte:head>
	<title>How to use OBS Background Removal on Arch Linux</title>
	<meta name="description" content="How to use OBS Background Removal on Arch Linux" />
</svelte:head>

<div class="text-column">
	<h1>Download and install on Arch Linux</h1>

	<p>
		<a
			href="https://github.com/obsproject/obs-studio/wiki/Unofficial-Linux-Builds#arch-linuxmanjaro"
			>Install the latest OBS by following the guide of unofficial builds</a
		>
	</p>

	<p>
		<a href="https://aur.archlinux.org/packages/obs-backgroundremoval"
			>Download PKGBUILD of obs-backgroundremoval from AUR</a
		>
	</p>

	<p>Build your package by running:</p>

	<pre>makepkg --syncdeps</pre>

	<p>Install your package by running:</p>

	<pre>makepkg --install</pre>

	<p>See the <a href="{base}/usage">usage page</a> for how to use the plugin</p>

	<p>All set!</p>

	<p><a href="{base}/">Back to the top page</a></p>
</div>
